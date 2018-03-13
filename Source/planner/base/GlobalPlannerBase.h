/*
 * GlobalPlannerBase.h
 *
 *  Created on: 2016年12月2日
 *      Author: seeing
 */

#ifndef _GLOBALPLANNERBASE_H_
#define _GLOBALPLANNERBASE_H_

///some dependency error makes this can not be removed
#include <DataSet/DataType/PoseStamped.h>

#include <transform/transform2d.h>
#include "../../costmap/CostmapWrapper.h"

namespace NS_Planner
{

  class GlobalPlannerBase
  {
  public:
    GlobalPlannerBase()
    {
    }
    ;
    virtual ~GlobalPlannerBase()
    {
    }
    ;

  public:
    void initialize(NS_CostMap::CostmapWrapper* costmap_)
    {
      costmap = costmap_;
      onInitialize();
    }
    ;

    virtual void
    onInitialize() = 0;

    virtual bool
    makePlan(const Pose2D& start,
             const Pose2D& goal,
             std::vector< Pose2D >& plan) = 0;

    virtual bool makePlan(const Pose2D& start,
                          const Pose2D& goal,
                          std::vector< Pose2D >& plan,
                          double& cost)
    {
      cost = 0;
      makePlan(start, goal, plan);
    }
    ;
  protected:
    NS_CostMap::CostmapWrapper* costmap;
  };

} /* namespace NS_Planner */

#endif /* NAVIGATION_PLANNER_BASE_GLOBALPLANNERBASE_H_ */
