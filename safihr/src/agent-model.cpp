/*
 * Copyright (C) 2014 INRA
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <vle/extension/decision/Agent.hpp>
#include <vle/extension/decision/Activity.hpp>
#include <vle/devs/Executive.hpp>
#include <vle/devs/ExecutiveDbg.hpp>
#include <vle/utils/Package.hpp>
#include <vle/utils/Rand.hpp>
#include <vle/utils/i18n.hpp>
#include <boost/unordered_map.hpp>
#include <fstream>
#include "global.hpp"
#include "gnuplot.hpp"
#include "crop.hpp"
#include "strategic.hpp"
#include "lu.hpp"

namespace safihr {

struct CropSoilState
{
    CropSoilState()
        : ru(0.0)
        , harvestable(false)
    {}

    double ru;
    bool harvestable;
};

struct CropSoilStateList
{
    void insert(const std::string& name)
    {
        lst.insert(
            std::make_pair <std::string, CropSoilState>(
                name, CropSoilState()));
    }

    const CropSoilState& get(const std::string& plot) const
    {
        const_iterator it = lst.find(plot);
        if (it == lst.end())
            throw vle::utils::ModellingError(
                vle::fmt("crop soil state: unknown plot %1%") % plot);

        return it->second;
    }

    CropSoilState& get(const std::string& plot)
    {
        iterator it = lst.find(plot);
        if (it == lst.end())
            throw vle::utils::ModellingError(
                vle::fmt("crop soil state: unknown plot %1%") % plot);

        return it->second;
    }

    typedef std::map <std::string, CropSoilState> container_type;
    typedef container_type::const_iterator const_iterator;
    typedef container_type::iterator iterator;

    container_type lst;
};

class Farmer : public vle::devs::Executive,
               public vle::extension::decision::KnowledgeBase
{
    typedef vle::extension::decision::Activities::result_t ActivityList;

    Plots m_rotation;
    Crops m_crops;
    LandUnits m_lus;

    /*
     * Create the DEVS model:
     *
     *
     *                  out  +---------+ out
     *          +------------+  meteo  +---------------------+
     *          |            +---------+                     |
     *          |                                            |
     *          |                                            |
     *          |                                            |
     *          |                                            |
     *   +-----------------------------------------------------------+
     *   |      |                                            |meteo  |
     *   |p1    |meteo                                 in +--v---+   |
     * +-v------v--+                  +------+------------>  p1  +---+  stade,ru
     * |           |                  |      |p1          +------+
     * |           |os              in|      |
     * |   Farmer  +------------------> O.S. |            +------+
     * |           |                  |      +------------>  p2  |-...
     * |           <------------------+      |p2        in+------+
     * +-----------+ack            out+------+               .
     *                                                       .
     *                                                       .
     *                                                    +------+
     *                                           . . .---->  pn  |-...
     *                                                    +------+
     */
    void farm_initialize()
    {
        createModelFromClass("class_meteo", meteo_model_name());
        addConnection(meteo_model_name(), "out",
                      farmer_model_name(), "meteo");

        createModelFromClass("class_os", operatingsystem_model_name());
        addConnection(farmer_model_name(), "os",
                      operatingsystem_model_name(), "in");
        addConnection(operatingsystem_model_name(), "out",
                      farmer_model_name(), "ack");

        for (size_t i = 0, e = m_lus.lus.size(); i != e; ++i) {
            std::string lu(landunit_model_name(i));

            addOutputPort(operatingsystem_model_name(), lu);
            addInputPort(farmer_model_name(), lu);

            createModelFromClass("class_p", lu);
            addConnection(meteo_model_name(), "out", lu, "meteo");
            addConnection(operatingsystem_model_name(), lu, lu, "in");
            addConnection(lu, "stade", farmer_model_name(), lu);
            addConnection(lu, "ru", farmer_model_name(), lu);
        }
    }

    void activities_observation();

    void register_outputs();
    void activity_out(const std::string& name,
                      const vle::extension::decision::Activity& activity,
                      vle::devs::ExternalEventList& lst);

    void register_facts();
    template <typename Container>
    void prediction_update(Container &con);
    void rain_fact(const vle::value::Value& value);
    void etp_fact(const vle::value::Value& value);
    void ru_fact(const std::string& port, const vle::value::Value& value);
    void harvestable_fact(const std::string& port, const vle::value::Value& value);

    void register_predicates();
    double get_sum_rain(int day_number) const;
    double get_sum_petp(int day_number) const;

    bool is_harvestable(const std::string& activity,
                        const std::string& rule,
                        const vle::extension::decision::PredicateParameters& param);
    bool is_penetrability_plot_valid(const std::string& activity, const std::string& rule,
                                     const vle::extension::decision::PredicateParameters& param);
    bool is_rain_quantity_valid(const std::string& activity, const std::string& rule,
                                const vle::extension::decision::PredicateParameters& param);
    bool is_rain_quantity_sum_valid(const std::string& activity, const std::string& rule,
                                    const vle::extension::decision::PredicateParameters& param);
    bool is_petp_quantity_sum_valid(const std::string& activity, const std::string& rule,
                                    const vle::extension::decision::PredicateParameters& param);
    bool is_etp_quantity_valid(const std::string& activity, const std::string& rule,
                               const vle::extension::decision::PredicateParameters& param);

    void strategic_assign_crop(const vle::devs::Time& time)
    {
        vle::utils::Package pack("safihr");

        for (size_t i = 0, e = m_rotation.size(); i != e; ++i) {
            m_crop_soil_state.insert((vle::fmt("p%1%") % i).str());

            std::string filename = (vle::fmt("ITK0-%1%.txt") %
                                    m_rotation.get(i).current_crop()).str();
            std::string filepath = pack.getDataFile(filename);
            std::ifstream ifs(filepath.c_str());
            if (not ifs.is_open())
                throw vle::utils::ModellingError(
                    vle::fmt("farmer: fail to open %1%") % filepath);

            try {
                plan().fill(ifs, time, (vle::fmt("_%1%_p%2%") % 0 % i).str());
            } catch (const std::exception& e) {
                throw vle::utils::ModellingError(
                    vle::fmt("farmer: fail to read %1% (plot: %2%): %3%") %
                    filepath % i % e.what());
            }

            DTraceModel(vle::fmt("agent assign crop: %1% to plot: %2%") %
                        m_rotation.get(i).current_crop() % i);
        }
    }

    /// TODO: work in progress
    /// search the next occurence of ITK

    void register_updates()
    {
        addUpdateFunctions(this) += U("itk_end", &Farmer::end_of_itk_update);
    }

    void end_of_itk_update(const std::string& name,
                           const vle::extension::decision::Activity& activity)
    {
        if (not activity.isInDoneState())
            return;

        std::string operation, crop, plot;
        int index, year;

        split_activity_name(name, &operation, &index, &crop, &year, &plot);

        // Cleanup previous crop harvestable boolean
        m_crop_soil_state.get(plot).harvestable = false;

        // Get the next crop and instantiate it.
        int plotid = boost::lexical_cast <int>(plot.substr(1, std::string::npos));
        const std::string& newcrop = m_rotation.get(plotid).next_crop();

        vle::utils::Package pack("safihr");
        std::string filepath = pack.getDataFile((vle::fmt("ITK-%1%.txt") % newcrop).str());

        try {
            std::ifstream ifs(filepath.c_str());

            DTraceModel(
                vle::fmt("agent assign winter crop %1% to plot %2% with load time %3%")
                % newcrop % plotid
                % vle::utils::DateTime::toJulianDay(m_time + 1));

            plan().fill(ifs,
                        m_time + 1,
                        (vle::fmt("_%1%_p%2%") % m_rotation.get(plotid).current
                         % plotid).str());
        } catch (const std::exception& e) {
            throw vle::utils::ModellingError(
                vle::fmt("farmer fails to append itk %1% from file %2% for plot %3% (%4%)")
                % newcrop % filepath % plotid % e.what());
        }
    }

public:
    Farmer(const vle::devs::ExecutiveInit& mdl,
           const vle::devs::InitEventList& evts)
        : vle::devs::Executive(mdl, evts)
        , m_prediction_size(7)
    {
        vle::utils::Package pack("safihr");

        {
            std::ifstream ifs(pack.getDataFile("Assolement-test.txt").c_str());
            if (!ifs.is_open())
                throw vle::utils::ModellingError(
                    vle::fmt("farmer: fails to open %1%") % "Assolement-test.txt");

            ifs >> m_rotation;
            if (ifs.fail())
                throw vle::utils::ModellingError(
                    vle::fmt("farmer: error while reading file %1%") %
                    "Assolement-test.txt");
        }

        {
            std::ifstream ifs(pack.getDataFile("Farm.txt").c_str());
            if (!ifs.is_open())
                throw vle::utils::ModellingError(
                    vle::fmt("farmer: fails to open %1%") % "Farm.txt");
            ifs >> m_lus;
            if (ifs.fail())
                throw vle::utils::ModellingError(
                    vle::fmt("farmer: error while reading file %1%") %
                    "Farm.txt");
        }

        {
            std::ifstream ifs(pack.getDataFile("Crop.txt").c_str());
            if (!ifs.is_open())
                throw vle::utils::ModellingError(
                    vle::fmt("farmer: fails to open %1%") % "Crop.txt");
            ifs >> m_crops;
            if (ifs.fail())
                throw vle::utils::ModellingError(
                    vle::fmt("farmer: error while reading file %1%") %
                    "Crop.txt");
        }

        if (evts.exist("prediction-size"))
            m_prediction_size = evts.getInt("prediction-size");

        if (m_prediction_size <= 0)
            throw vle::utils::ModellingError(
                "farmer: prediction size is too small");

        m_rain_prediction.resize(m_prediction_size + 2);
        m_etp_prediction.resize(m_prediction_size + 2);

        register_updates();
        register_predicates();
        register_facts();
        register_outputs();
    }

    virtual ~Farmer()
    {
    }

    virtual vle::devs::Time init(const vle::devs::Time& time)
    {
        farm_initialize();
        strategic_assign_crop(time);

        mState = Output;
        m_time = time;
        mNextChangeTime = processChanges(time);

        return 0.0;
    }

    virtual void finish()
    {
        build_grantt_observation(activities());
    }

    virtual void output(const vle::devs::Time& time,
                        vle::devs::ExternalEventList& output) const
    {
        (void)time;

        if (mState == Output) {

            {
                const Farmer::ActivityList& lst = latestStartedActivities();
                Farmer::ActivityList::const_iterator it = lst.begin();
                for (; it != lst.end(); ++it) {
                    (*it)->second.output((*it)->first, output);
                }
            }
            {
                const Farmer::ActivityList& lst = latestFailedActivities();
                Farmer::ActivityList::const_iterator it = lst.begin();
                for (; it != lst.end(); ++it) {
                    (*it)->second.output((*it)->first, output);
                }
            }
            {
                const Farmer::ActivityList& lst = latestDoneActivities();
                Farmer::ActivityList::const_iterator it = lst.begin();
                for (; it != lst.end(); ++it) {
                    (*it)->second.output((*it)->first, output);
                }
            }
            {
                const Farmer::ActivityList& lst = latestEndedActivities();
                Farmer::ActivityList::const_iterator it = lst.begin();
                for (; it != lst.end(); ++it) {
                    (*it)->second.output((*it)->first, output);
                }
            }
        }
    }

    virtual vle::devs::Time timeAdvance() const
    {
        switch (mState) {
        case Init:
        case Output:
        case UpdateFact:
            return 0.0;
        case Process:
            if (mNextChangeTime.second == vle::devs::negativeInfinity or
                mNextChangeTime.first == true or
                haveActivityInLatestActivitiesLists()) {
                return 0.0;
            } else {
                return mNextChangeTime.second - m_time;
            }
        }

        throw std::logic_error("unknown state");
    }

    virtual void internalTransition(const vle::devs::Time& time)
    {
        m_time = time;

        switch (mState) {
        case Output:
            clearLatestActivitiesLists();
        case Init:
        case UpdateFact:
            mNextChangeTime = processChanges(time);
            mState = Process;
            break;
        case Process:
            mState = Output;
            break;
        }
    }

    virtual void externalTransition(const vle::devs::ExternalEventList& events,
                                    const vle::devs::Time& time)
    {
        m_time = time;

        for (vle::devs::ExternalEventList::const_iterator it = events.begin();
             it != events.end(); ++it) {
            const std::string& port((*it)->getPortName());
            const vle::value::Map& atts = (*it)->getAttributes();

            if (port == "ack") {
                const std::string& activity(atts.getString("activity"));
                const std::string& order(atts.getString("order"));

                TraceModel(vle::fmt("farmer receives ack (%1% - %2%)") % activity % order);

                if (order == "done") {
                    setActivityDone(activity, time);
                } else if (order == "fail") {
                    setActivityFailed(activity, time);
                } else {
                    throw vle::utils::ModellingError(
                        vle::fmt(_("Decision: unknown order `%1%'")) % order);
                }
            } else if (port == "meteo") {
                TraceModel("farmer receives meteo");
                applyFact("rain", *atts.get("rain"));
                applyFact("etp", *atts.get("etp"));
            } else if (port[0] == 'p') {
                TraceModel("farmer receives ru");
                if (atts.exist("ru"))
                    ru_fact(port, atts);
                if (atts.exist("harvestable"))
                    harvestable_fact(port, atts);
            } else {
                assert(false);
                vle::value::Map::const_iterator jt = atts.value().find("value");
                if (jt == atts.end()) {
                    jt = atts.value().find("init");
                }

                if (jt == atts.end() or not jt->second) {
                    throw vle::utils::ModellingError(
                        vle::fmt(_("Decision: no value in this message: `%1%'"))
                        % (*it));
                }

                applyFact(port, *jt->second);
            }
        }

        mState = UpdateFact;
    }

private:
    enum State
    {
        Init,
        Process,
        UpdateFact,
        Output
    };

    vle::extension::decision::KnowledgeBase::Result mNextChangeTime;
    vle::devs::Time m_time;
    vle::utils::Rand m_rand;
    State mState;

    CropSoilStateList m_crop_soil_state;

    std::deque <double> m_rain;
    std::deque <double> m_etp;
    std::vector <double> m_rain_prediction;
    std::vector <double> m_etp_prediction;
    size_t m_prediction_size;
};


//
// Outputs
//

void Farmer::register_outputs()
{
    addOutputFunctions(this) +=
        O("out", &Farmer::activity_out);
}

//
// Facts
//

void Farmer::activity_out(const std::string& name,
                          const vle::extension::decision::Activity& activity,
                          vle::devs::ExternalEventList& lst)
{
    (void)activity;

    if (activity.isInStartedState()) {
        std::string crop, plot, order = "other";
        split_activity_name(name, NULL, NULL, &crop, NULL, &plot);

        if (name.find("Seeding") != std::string::npos or name.find("Sowing") != std::string::npos)
            order = "sow";
        else if (name.find("Harvest") != std::string::npos)
            order = "harvest";

        vle::devs::ExternalEvent *evt = new vle::devs::ExternalEvent("os");
        evt->putAttribute("p", new vle::value::String(plot));
        evt->putAttribute("order", new vle::value::String(order));
        evt->putAttribute("crop", new vle::value::String(crop));
        evt->putAttribute("activity", new vle::value::String(name));

        // Get speed of this activity and the size of the plot to build
        // the duration. Default, the duration of any activity is one day.
        double speed = 1.0;
        if (activity.speed() > 0.0)
            speed = m_lus.lus.at(split_plot_name(plot)).sau / activity.speed();

        evt->putAttribute("duration", new vle::value::Double(speed));

        DTraceModel(vle::fmt("activity %1% sends output to %2% order %3% "
                             "for a duration of %4% (%5%/%6%)")
                    % name % plot % order % speed
                    % m_lus.lus.at(split_plot_name(plot)).sau
                    % activity.speed());

        lst.push_back(evt);
    }
}

void Farmer::register_facts()
{
    addFacts(this) +=
        F("rain", &Farmer::rain_fact),
        F("etp", &Farmer::etp_fact);
}

template <typename Container>
void Farmer::prediction_update(Container &con)
{
    for (size_t i = 2, e = con.size(); i != e; ++i) {
        double sum = std::accumulate(con.begin(), con.begin() + i, 0.0);
        double mean = sum / i;
        double diff = std::abs(con[i - 2] - con[i - 1]);

        con[i] = m_rand.normal(mean, diff == 0.0 ? 1.0 : 0.0);
    }
}

void Farmer::rain_fact(const vle::value::Value& value)
{
    double rain_quantity =  vle::value::toDouble(value);

    m_rain.push_front(rain_quantity);

    if (m_time)
        m_rain_prediction[0] = m_rain_prediction[1];
    else
        m_rain_prediction[0] = rain_quantity;

    m_rain_prediction[1] = rain_quantity;

    prediction_update(m_rain_prediction);

    TraceModel("rain_fact updated");
}

void Farmer::etp_fact(const vle::value::Value& value)
{
    double etp_quantity = vle::value::toDouble(value);

    m_etp.push_front(etp_quantity);

    if (m_time)
        m_etp_prediction[0] = m_etp_prediction[1];
    else
        m_etp_prediction[0] = etp_quantity;

    m_etp_prediction[1] = etp_quantity;

    prediction_update(m_etp_prediction);

    TraceModel("etp_fact updated");
}

void Farmer::ru_fact(const std::string& port, const vle::value::Value& value)
{
    m_crop_soil_state.get(port).ru = vle::value::toMapValue(value).getDouble("ru");

    DTraceModel(vle::fmt("ru_fact for %1%=%2%") % port % m_crop_soil_state.get(port).ru);
}

void Farmer::harvestable_fact(const std::string& port, const vle::value::Value& value)
{
    (void)value;

    m_crop_soil_state.get(port).harvestable = true;

    DTraceModel(vle::fmt("harvestable_fact for %1%=%2%") % port % true);
}

//
// Predicates
//

void Farmer::register_predicates()
{
    addPredicates(this) +=
        P("harvestable", &Farmer::is_harvestable),
        P("penetrability", &Farmer::is_penetrability_plot_valid),
        P("rain", &Farmer::is_rain_quantity_valid),
        P("sum_rain", &Farmer::is_rain_quantity_sum_valid),
        P("sum_R-PET", &Farmer::is_petp_quantity_sum_valid),
        P("etp", &Farmer::is_etp_quantity_valid);
}

bool Farmer::is_harvestable(const std::string& activity,
                            const std::string& rule,
                            const vle::extension::decision::PredicateParameters& param)
{
    (void)rule;
    (void)param;

    std::string plot;
    split_activity_name(activity, NULL, NULL, NULL, NULL, &plot);

    return m_crop_soil_state.get(plot).harvestable;
}

bool Farmer::is_penetrability_plot_valid(const std::string& activity,
                                         const std::string& rule,
                                         const vle::extension::decision::PredicateParameters& param)
{
    (void)rule;

    std::string op = param.getString("penetrability_operator");
    double value = param.getDouble("penetrability_threshold");

    std::string plot;
    split_activity_name(activity, NULL, NULL, NULL, NULL, &plot);

    if (op == "<")
        return m_crop_soil_state.get(plot).ru < value;

    if (op == ">=")
        return m_crop_soil_state.get(plot).ru >= value;

    if (op == "=") {
        DTraceModel(vle::fmt("penetrability %1% %2% == %3% (%4%)")
                    % op % value % m_crop_soil_state.get(plot).ru
                    % (is_almost_equal(m_crop_soil_state.get(plot).ru, value)));

        return is_almost_equal(m_crop_soil_state.get(plot).ru, value);
    }

    throw vle::utils::ModellingError(
        vle::fmt("farmer predicate penetrability: unknown operator %1%") % op);
}

bool Farmer::is_rain_quantity_valid(const std::string& activity,
                                    const std::string& rule,
                                    const vle::extension::decision::PredicateParameters& param)
{
    (void)activity;
    (void)rule;

    std::string op = param.getString("rain_operator");
    double value = param.getDouble("rain_threshold");

    if (op == "<=")
        return m_rain_prediction[1] <= value;

    if (op == ">=")
        return m_rain_prediction[1] >= value;

    if (op == "=")
        return is_almost_equal(m_rain_prediction[1], value);

    throw vle::utils::ModellingError(
        vle::fmt("farmer predicate rain: unknown operator %1%") % op);
}

double Farmer::get_sum_rain(int day_number) const
{
    if (day_number <= 2)
        return (m_rain_prediction[0] + m_rain_prediction[1]) / 2.0;

    return std::accumulate(m_rain.begin(), m_rain.begin() + day_number,
                           0.0) / day_number;
}

double Farmer::get_sum_petp(int day_number) const
{
    std::vector <double> petp(day_number);

    std::transform(m_rain.begin(), m_rain.begin() + day_number,
                   m_etp.begin(),
                   petp.begin(),
                   std::minus <double>());

    return std::accumulate(petp.begin(), petp.end(),
                           0.0) / (double)day_number;
}

bool Farmer::is_rain_quantity_sum_valid(const std::string& activity,
                                        const std::string& rule,
                                        const vle::extension::decision::PredicateParameters& param)
{
    (void)activity;
    (void)rule;

    std::string op = param.getString("sum_rain_operator");
    double number = param.getDouble("sum_rain_number");
    double value = param.getDouble("sum_rain_threshold");

    if (m_rain.size() < static_cast <size_t>(number)) {
        DTraceModel(vle::fmt("Farmer: not enough rain in memory (%1%/%2%)")
                    % number % m_rain.size());
        return false;
    }

    if (op == "<=")
        return get_sum_rain(static_cast <int>(number)) <= value;

    throw vle::utils::ModellingError(
        vle::fmt("farmer predicate sum_rain: unknown operator %1%") % op);
}

bool Farmer::is_petp_quantity_sum_valid(const std::string& activity,
                                        const std::string& rule,
                                        const vle::extension::decision::PredicateParameters& param)
{
    (void)activity;
    (void)rule;

    std::string op = param.getString("sum_R-PET_operator");
    double day_number = param.getDouble("sum_R-PET_number");
    double value = param.getDouble("sum_R-PET_threshold");

    if (day_number < 0 or m_etp.size() < static_cast <size_t>(day_number)) {
        DTraceModel(vle::fmt("Farmer: not enough etp in memory (%1%/%2%)")
                    % day_number % m_etp.size());
        return false;
    }

    if (op == "<=")
        return get_sum_petp(static_cast <int>(day_number)) <= value;

    throw vle::utils::ModellingError(
        vle::fmt("farmer predicate sum_R-PET: unknown operator %1%") % op);
}

bool Farmer::is_etp_quantity_valid(const std::string& activity,
                                   const std::string& rule,
                                   const vle::extension::decision::PredicateParameters& param)
{
    (void)activity;
    (void)rule;

    std::string op = param.getString("etp_operator");
    double value = param.getDouble("etp_threshold");

    if (op == "<=")
        return m_rain_prediction[1] <= value;

    throw vle::utils::ModellingError(
        vle::fmt("farmer predicate rain: unknown operator %1%") % op);
}

} // namespace safihr

DECLARE_EXECUTIVE_DBG(safihr::Farmer)
