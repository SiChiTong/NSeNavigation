#ifndef _BASE_LOCAL_PLANNER_FOOTPRINT_HELPER_H_
#define _BASE_LOCAL_PLANNER_FOOTPRINT_HELPER_H_

#include <vector>

#include "../../../../costmap/costmap_2d/CostMap2D.h"
#include <type/point2d.h>
using namespace sgbot;
namespace NS_Planner
{
  class FootprintHelper
  {
  public:
    FootprintHelper();
    virtual
    ~FootprintHelper();

    /**
     * @brief  Used to get the cells that make up the footprint of the robot
     * @param x_i The x position of the robot
     * @param y_i The y position of the robot
     * @param theta_i The orientation of the robot
     * @param  fill If true: returns all cells in the footprint of the robot. If false: returns only the cells that make up the outline of the footprint.
     * @return The cells that make up either the outline or entire footprint of the robot depending on fill
     */
    std::vector< Point2D >
    getFootprintCells(std::vector<float> pos,
                      std::vector< Point2D > footprint_spec,
                      const NS_CostMap::Costmap2D&, bool fill);

    /**
     * @brief  Use Bresenham's algorithm to trace a line between two points in a grid
     * @param  x0 The x coordinate of the first point
     * @param  x1 The x coordinate of the second point
     * @param  y0 The y coordinate of the first point
     * @param  y1 The y coordinate of the second point
     * @param  pts Will be filled with the cells that lie on the line in the grid
     */
    void
    getLineCells(int x0, int x1, int y0, int y1,
                 std::vector< Point2D >& pts);

    /**
     * @brief Fill the outline of a polygon, in this case the robot footprint, in a grid
     * @param footprint The list of cells making up the footprint in the grid, will be modified to include all cells inside the footprint
     */
    void
    getFillCells(std::vector< Point2D >& footprint);
  };

} /* namespace base_local_planner */
#endif /* FOOTPRINT_HELPER_H_ */
