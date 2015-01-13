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
#include <fstream>
#include "global.hpp"
#include "strategic.hpp"
#include "lu.hpp"

namespace safihr {

class Farmer : public vle::devs::Executive,
               public vle::extension::decision::KnowledgeBase
{
    typedef vle::extension::decision::Activities::result_t ActivityList;

    CropRotation m_rotation;
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

    void register_facts();
    template <typename Container>
    void prediction_update(Container &con);
    void rain_fact(const vle::value::Value& value);
    void etp_fact(const vle::value::Value& value);
    void ru_fact(const vle::value::Value& value);

    void register_predicates();
    std::string get_landunit_from_activity_name(const std::string& activity);
    double get_ru_from_activity_name(const std::string& activity);
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
        std::ifstream ifs(pack.getDataFile("ITK-BS.txt"));
        if (not ifs.is_open())
            throw vle::utils::ModellingError(
                vle::fmt("farmer: fail to open %1%") % "ITK-BS.txt");


        m_crop_soil_state.insert(
            std::make_pair <std::string, crop_soil_state>(
                "p0", crop_soil_state(0.0, false)));

        plan().fill(ifs, time, "_p0");
    }

public:
    Farmer(const vle::devs::ExecutiveInit& mdl,
           const vle::devs::InitEventList& evts)
        : vle::devs::Executive(mdl, evts)
        , m_prediction_size(5)
    {
        vle::utils::Package pack("safihr");

        {
            std::ifstream ifs(pack.getDataFile("Assolement-test.txt"));
            if (!ifs.is_open())
                throw vle::utils::ModellingError(
                    vle::fmt("farmer: fail to open %1%") % "Assolement-test.txt");

            ifs >> m_rotation;
        }

        {
            std::ifstream ifs(pack.getDataFile("Farm.txt"));
            if (!ifs.is_open())
                throw vle::utils::ModellingError(
                    vle::fmt("farmer: fail to open %1%") % "Farm.txt");
            ifs >> m_lus;
        }

        if (evts.exist("prediction-size"))
            m_prediction_size = evts.getInt("prediction-size");

        if (m_prediction_size <= 0)
            throw vle::utils::ModellingError(
                "farmer: prediction size is too small");

        m_rain_prediction.resize(m_prediction_size + 2);
        m_etp_prediction.resize(m_prediction_size + 2);

        register_predicates();
        register_facts();
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
                const std::string& activity(atts.getString("name"));
                const std::string& order(atts.getString("value"));

                if (order == "done") {
                    setActivityDone(activity, time);
                } else if (order == "fail") {
                    setActivityFailed(activity, time);
                } else {
                    throw vle::utils::ModellingError(
                        vle::fmt(_("Decision: unknown order `%1%'")) % order);
                }
            } else if (port == "meteo") {
                applyFact("rain", *atts.get("rain"));
                applyFact("etp", *atts.get("etp"));
            } else if (port[0] == 'p') {
                if (atts.exist("ru")) {
                    vle::value::Map mp;
                    mp.add("p", new vle::value::String(port));
                    mp.add("ru", new vle::value::Double(atts.getDouble("ru")));
                    applyFact("ru", mp);
                }
                if (atts.exist("harvestable")) {
                    applyFact("ru", *atts.get("harvestable"));
                }
            } else {
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

    struct crop_soil_state
    {
        crop_soil_state(double _ru, bool _harvestable)
            : ru(_ru)
            , harvestable(_harvestable)
        {}

        double ru;
        bool harvestable;
    };

    typedef std::map <std::string, crop_soil_state> crop_soil_state_list;
    crop_soil_state_list m_crop_soil_state;

    std::deque <double> m_rain;
    std::deque <double> m_etp;
    std::vector <double> m_rain_prediction;
    std::vector <double> m_etp_prediction;
    size_t m_prediction_size;
};


//
// Facts
//

void Farmer::register_facts()
{
    addFacts(this) +=
        F("rain", &Farmer::rain_fact),
        F("etp", &Farmer::etp_fact),
        F("ru", &Farmer::ru_fact);
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
}

void Farmer::ru_fact(const vle::value::Value& value)
{
    std::string p = vle::value::toMapValue(value).getString("p");
    double ru = vle::value::toMapValue(value).getDouble("ru");

    crop_soil_state_list::iterator it = m_crop_soil_state.find(p);
    if (it == m_crop_soil_state.end())
        throw std:: invalid_argument(
            (vle::fmt("unknown landunit %1%") % p).str());

    it->second.ru = ru;
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
        P("d_rain", &Farmer::is_rain_quantity_sum_valid),
        P("d_petp", &Farmer::is_petp_quantity_sum_valid),
        P("etp", &Farmer::is_etp_quantity_valid);
}

std::string Farmer::get_landunit_from_activity_name(const std::string& activity)
{
    std::string::size_type pos = activity.find_last_of("_");
    if (pos == std::string::npos)
        throw vle::utils::ModellingError(
            vle::fmt("farmer: unknown activity %1%") % activity);

    return activity.substr(pos + 1, std::string::npos);
}

double Farmer::get_ru_from_activity_name(const std::string& activity)
{
    std::string landunit = get_landunit_from_activity_name(activity);

    crop_soil_state_list::const_iterator it = m_crop_soil_state.find(landunit);
    if (it == m_crop_soil_state.end())
        throw vle::utils::ModellingError(
            vle::fmt("farmer: unknown landunit %1%") % landunit);

    return it->second.ru;
}

bool Farmer::is_harvestable(const std::string& activity,
                            const std::string& rule,
                            const vle::extension::decision::PredicateParameters& param)
{
    /* TODO: need to split activity to take the plot identifier and test
       the crop state. */

    return false;
}

bool Farmer::is_penetrability_plot_valid(const std::string& activity,
                                         const std::string& rule,
                                         const vle::extension::decision::PredicateParameters& param)
{
    (void)rule;

    std::string op = param.getString("penetrability_operator");
    double value = param.getDouble("penetrability_param");

    if (op == "<")
        return get_ru_from_activity_name(activity) < value;

    if (op == ">=")
        return get_ru_from_activity_name(activity) >= value;

    if (op == "=")
        return is_almost_equal(get_ru_from_activity_name(activity), value);

    throw vle::utils::ModellingError("farmer: unknown penetrability_operator");
}

bool Farmer::is_rain_quantity_valid(const std::string& activity,
                                    const std::string& rule,
                                    const vle::extension::decision::PredicateParameters& param)
{
    (void)activity;
    (void)rule;

    double value = param.getDouble("rain_param");

    return m_rain_prediction[1] <= value;
}

double Farmer::get_sum_rain(int day_number) const
{
    if (day_number <= 2)
        return (m_rain_prediction[0] + m_rain_prediction[1]) / 2.0;

    if (m_rain.size() < static_cast <size_t>(day_number))
        throw vle::utils::ModellingError(
            "farmer: not enough memory for rain");

    return std::accumulate(m_rain.begin(), m_rain.begin() + day_number,
                           0.0) / day_number;
}

double Farmer::get_sum_petp(int day_number) const
{
    if (day_number < 0 or m_etp.size() < static_cast <size_t>(day_number))
        throw vle::utils::ModellingError(
            "farmer: not enough memory for p-etp");

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

    double day_number = param.getDouble("rain_day_number");
    double value = param.getDouble("rain_sum");

    return get_sum_rain(static_cast <int>(day_number)) <= value;
}

bool Farmer::is_petp_quantity_sum_valid(const std::string& activity,
                                        const std::string& rule,
                                        const vle::extension::decision::PredicateParameters& param)
{
    (void)activity;
    (void)rule;

    double day_number = param.getDouble("rain_day_number");
    double value = param.getDouble("p-etp_sum");

    return get_sum_petp(static_cast <int>(day_number)) <= value;
}

bool Farmer::is_etp_quantity_valid(const std::string& activity,
                                   const std::string& rule,
                                   const vle::extension::decision::PredicateParameters& param)
{
    (void)activity;
    (void)rule;

    return m_etp_prediction[1] > param.getDouble("etp_param");
}


} // namespace safihr

DECLARE_EXECUTIVE_DBG(safihr::Farmer)
