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
#include <vle/devs/DynamicsDbg.hpp>
#include <sstream>

namespace vd = vle::devs;
namespace vv = vle::value;
namespace vmd = vle::extension::decision;

namespace vle { namespace examples { namespace decision {

class SimpleAgent: public vmd::Agent
{
public:
    SimpleAgent(const vd::DynamicsInit& mdl, const vd::InitEventList& evts)
        : vmd::Agent(mdl, evts), mStart(false)
    {
        addFact("start", boost::bind(&SimpleAgent::start, this, _1));

        vmd::Rule& r = addRule("rule");
        r.add(boost::bind(&SimpleAgent::isStarted, this));

        vmd::Rule& r2 = addRule("rule2");
        r2.add(boost::bind(&SimpleAgent::alwaysFalse, this));

        vmd::Activity& b = addActivity("B", 0.0, 10.0);
        b.addRule("rule2", r2);

        vmd::Activity& a = addActivity("A", 0.0, 10.0);
        a.addRule("rule", r);

        addStartToStartConstraint("A", "B", 3.0);
    }

    virtual ~SimpleAgent()
    {
    }

    void start(const vle::value::Value& val)
    {
        if (not mStart) {
            mStart = val.toBoolean().value();
        }
    }

    bool isStarted() const
    {
        return mStart;
    }

    bool alwaysFalse() const
    {
        return false;
    }

    virtual vv::Value* observation(const vd::ObservationEvent& evt) const
    {
        if (evt.onPort("text")) {
            std::ostringstream out;
            out << *this;

            return new vv::String(out.str());
        }

        return 0;
    }

private:
    bool mStart;
};

}}} // namespace vle examples decision

DECLARE_DYNAMICS_DBG(vle::examples::decision::SimpleAgent)
