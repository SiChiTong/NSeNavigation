#include "CostMap2D.h"

#include <cstdio>

using namespace std;

namespace NS_CostMap
{
  Costmap2D::Costmap2D(unsigned int cells_size_x, unsigned int cells_size_y,
                       float resolution, float origin_x, float origin_y,
                       unsigned char default_value)
      : size_x_(cells_size_x), size_y_(cells_size_y), resolution_(resolution),
        origin_x_(origin_x), origin_y_(origin_y), costmap_(NULL),
        default_value_(default_value)
  {
    access_ = new mutex_t();


    // create the costmap
    initMaps(size_x_, size_y_);
    resetMaps();
  }

  void Costmap2D::deleteMaps()
  {
    // clean up data
    boost::unique_lock< mutex_t > lock(*access_);
    delete[] costmap_;
    costmap_ = NULL;
  }

  void Costmap2D::initMaps(unsigned int size_x, unsigned int size_y)
  {
    boost::unique_lock< mutex_t > lock(*access_);
    delete[] costmap_;
    costmap_ = new unsigned char[size_x * size_y];
  }

  void Costmap2D::resizeMap(unsigned int size_x, unsigned int size_y,
                            float resolution, float origin_x, float origin_y)
  {
    size_x_ = size_x;
    size_y_ = size_y;
    resolution_ = resolution;
    origin_x_ = origin_x;
    origin_y_ = origin_y;

    initMaps(size_x, size_y);

    // reset our maps to have no information
    resetMaps();
  }

  void Costmap2D::resetMaps()
  {
    boost::unique_lock< mutex_t > lock(*access_);
    memset(costmap_, default_value_, size_x_ * size_y_ * sizeof(unsigned char));
  }

  void Costmap2D::resetMap(unsigned int x0, unsigned int y0, unsigned int xn,
                           unsigned int yn)
  {
    boost::unique_lock< mutex_t > lock(*(access_));
    unsigned int len = xn - x0;
    for(unsigned int y = y0 * size_x_ + x0; y < yn * size_x_ + x0; y += size_x_)
      memset(costmap_ + y, default_value_, len * sizeof(unsigned char));
  }

  bool Costmap2D::copyCostmapWindow(const Costmap2D& map, float win_origin_x,
                                    float win_origin_y, float win_size_x,
                                    float win_size_y)
  {
    // check for self windowing
    if(this == &map)
    {
      // ROS_ERROR("Cannot convert this costmap into a window of itself");
      return false;
    }

    // clean up old data
    deleteMaps();

    // compute the bounds of our new map
    unsigned int lower_left_x, lower_left_y, upper_right_x, upper_right_y;
    if(!worldToMap(win_origin_x, win_origin_y, lower_left_x, lower_left_y) || !worldToMap(
        win_origin_x + win_size_x, win_origin_y + win_size_y, upper_right_x,
        upper_right_y))
    {
      // ROS_ERROR("Cannot window a map that the window bounds don't fit inside of");
      return false;
    }

    size_x_ = upper_right_x - lower_left_x;
    size_y_ = upper_right_y - lower_left_y;
    resolution_ = map.resolution_;
    origin_x_ = win_origin_x;
    origin_y_ = win_origin_y;

    // initialize our various maps and reset markers for inflation
    initMaps(size_x_, size_y_);

    // copy the window of the static map and the costmap that we're taking
    copyMapRegion(map.costmap_, lower_left_x, lower_left_y, map.size_x_,
                  costmap_, 0, 0, size_x_, size_x_, size_y_);
    return true;
  }

  Costmap2D&
  Costmap2D::operator=(const Costmap2D& map)
  {
    // check for self assignement
    if(this == &map)
      return *this;

    // clean up old data
    deleteMaps();

    size_x_ = map.size_x_;
    size_y_ = map.size_y_;
    resolution_ = map.resolution_;
    origin_x_ = map.origin_x_;
    origin_y_ = map.origin_y_;

    // initialize our various maps
    initMaps(size_x_, size_y_);

    // copy the cost map
    memcpy(costmap_, map.costmap_, size_x_ * size_y_ * sizeof(unsigned char));

    return *this;
  }

  Costmap2D::Costmap2D(const Costmap2D& map)
      : costmap_(NULL)
  {
    access_ = new mutex_t();
    *this = map;
  }

// just initialize everything to NULL by default
  Costmap2D::Costmap2D()
      : size_x_(0), size_y_(0), resolution_(0.0), origin_x_(0.0),
        origin_y_(0.0), costmap_(NULL)
  {
    access_ = new mutex_t();
  }

  Costmap2D::~Costmap2D()
  {
    deleteMaps();
    delete access_;
  }

  unsigned int Costmap2D::cellDistance(float world_dist)
  {
    float cells_dist = max(0.0f, ceil(world_dist / resolution_));
    return (unsigned int)cells_dist;
  }

  unsigned char*
  Costmap2D::getCharMap() const
  {
    return costmap_;
  }

  unsigned char Costmap2D::getCost(unsigned int mx, unsigned int my) const
  {
    return costmap_[getIndex(mx, my)];
  }

  void Costmap2D::setCost(unsigned int mx, unsigned int my, unsigned char cost)
  {
    costmap_[getIndex(mx, my)] = cost;
  }


  void Costmap2D::updateOrigin(float new_origin_x, float new_origin_y)
  {
    // project the new origin into the grid
    int cell_ox, cell_oy;
    cell_ox = int((new_origin_x - origin_x_) / resolution_);
    cell_oy = int((new_origin_y - origin_y_) / resolution_);

    // compute the associated world coordinates for the origin cell
    // because we want to keep things grid-aligned
    float new_grid_ox, new_grid_oy;
    new_grid_ox = origin_x_ + cell_ox * resolution_;
    new_grid_oy = origin_y_ + cell_oy * resolution_;

    // To save casting from unsigned int to int a bunch of times
    int size_x = size_x_;
    int size_y = size_y_;

    // we need to compute the overlap of the new and existing windows
    int lower_left_x, lower_left_y, upper_right_x, upper_right_y;
    lower_left_x = min(max(cell_ox, 0), size_x);
    lower_left_y = min(max(cell_oy, 0), size_y);
    upper_right_x = min(max(cell_ox + size_x, 0), size_x);
    upper_right_y = min(max(cell_oy + size_y, 0), size_y);

    unsigned int cell_size_x = upper_right_x - lower_left_x;
    unsigned int cell_size_y = upper_right_y - lower_left_y;

    // we need a map to store the obstacles in the window temporarily
    unsigned char* local_map = new unsigned char[cell_size_x * cell_size_y];

    // copy the local window in the costmap to the local map
    copyMapRegion(costmap_, lower_left_x, lower_left_y, size_x_, local_map, 0,
                  0, cell_size_x, cell_size_x, cell_size_y);

    // now we'll set the costmap to be completely unknown if we track unknown space
    resetMaps();

    // update the origin with the appropriate world coordinates
    origin_x_ = new_grid_ox;
    origin_y_ = new_grid_oy;

    // compute the starting cell location for copying data back in
    int start_x = lower_left_x - cell_ox;
    int start_y = lower_left_y - cell_oy;

    // now we want to copy the overlapping information back into the map, but in its new location
    copyMapRegion(local_map, 0, 0, cell_size_x, costmap_, start_x, start_y,
                  size_x_, cell_size_x, cell_size_y);

    // make sure to clean up
    delete[] local_map;
  }

//  bool Costmap2D::setConvexPolygonCost(
//      const std::vector< Point2D >& polygon,
//      unsigned char cost_value)
//  {
//    // we assume the polygon is given in the global_frame... we need to transform it to map coordinates
//    std::vector< MapLocation > map_polygon;
//    for(unsigned int i = 0; i < polygon.size(); ++i)
//    {
//      MapLocation loc;
//      if(!worldToMap(polygon[i].x, polygon[i].y, loc.x, loc.y))
//      {
//        // ("Polygon lies outside map bounds, so we can't fill it");
//        return false;
//      }
//      map_polygon.push_back(loc);
//    }
//
//    std::vector< MapLocation > polygon_cells;
//
//    // get the cells that fill the polygon
//    convexFillCells(map_polygon, polygon_cells);
//
//    // set the cost of those cells
//    for(unsigned int i = 0; i < polygon_cells.size(); ++i)
//    {
//      unsigned int index = getIndex(polygon_cells[i].x, polygon_cells[i].y);
//      costmap_[index] = cost_value;
//    }
//    return true;
//  }

//  void Costmap2D::polygonOutlineCells(const std::vector< MapLocation >& polygon,
//                                      std::vector< MapLocation >& polygon_cells)
//  {
//    PolygonOutlineCells cell_gatherer(*this, costmap_, polygon_cells);
//    for(unsigned int i = 0; i < polygon.size() - 1; ++i)
//    {
//      raytraceLine(cell_gatherer, polygon[i].x, polygon[i].y, polygon[i + 1].x,
//                   polygon[i + 1].y);
//    }
//    if(!polygon.empty())
//    {
//      unsigned int last_index = polygon.size() - 1;
//      // we also need to close the polygon by going from the last point to the first
//      raytraceLine(cell_gatherer, polygon[last_index].x, polygon[last_index].y,
//                   polygon[0].x, polygon[0].y);
//    }
//  }
//
//  void Costmap2D::convexFillCells(const std::vector< MapLocation >& polygon,
//                                  std::vector< MapLocation >& polygon_cells)
//  {
//    // we need a minimum polygon of a triangle
//    if(polygon.size() < 3)
//      return;
//
//    // first get the cells that make up the outline of the polygon
//    polygonOutlineCells(polygon, polygon_cells);
//
//    // quick bubble sort to sort points by x
//    MapLocation swap;
//    unsigned int i = 0;
//    while(i < polygon_cells.size() - 1)
//    {
//      if(polygon_cells[i].x > polygon_cells[i + 1].x)
//      {
//        swap = polygon_cells[i];
//        polygon_cells[i] = polygon_cells[i + 1];
//        polygon_cells[i + 1] = swap;
//
//        if(i > 0)
//          --i;
//      }
//      else
//        ++i;
//    }
//
//    i = 0;
//    MapLocation min_pt;
//    MapLocation max_pt;
//    unsigned int min_x = polygon_cells[0].x;
//    unsigned int max_x = polygon_cells[polygon_cells.size() - 1].x;
//
//    // walk through each column and mark cells inside the polygon
//    for(unsigned int x = min_x; x <= max_x; ++x)
//    {
//      if(i >= polygon_cells.size() - 1)
//        break;
//
//      if(polygon_cells[i].y < polygon_cells[i + 1].y)
//      {
//        min_pt = polygon_cells[i];
//        max_pt = polygon_cells[i + 1];
//      }
//      else
//      {
//        min_pt = polygon_cells[i + 1];
//        max_pt = polygon_cells[i];
//      }
//
//      i += 2;
//      while(i < polygon_cells.size() && polygon_cells[i].x == x)
//      {
//        if(polygon_cells[i].y < min_pt.y)
//          min_pt = polygon_cells[i];
//        else if(polygon_cells[i].y > max_pt.y)
//          max_pt = polygon_cells[i];
//        ++i;
//      }
//
//      MapLocation pt;
//      // loop though cells in the column
//      for(unsigned int y = min_pt.y; y < max_pt.y; ++y)
//      {
//        pt.x = x;
//        pt.y = y;
//        polygon_cells.push_back(pt);
//      }
//    }
//  }
//


  bool Costmap2D::saveMap(std::string file_name)
  {
    FILE *fp = fopen(file_name.c_str(), "w");

    if(!fp)
    {
      return false;
    }

    fprintf(fp, "P2\n%u\n%u\n%u\n", size_x_, size_y_, 0xff);
    for(unsigned int iy = 0; iy < size_y_; iy++)
    {
      for(unsigned int ix = 0; ix < size_x_; ix++)
      {
        unsigned char cost = getCost(ix, iy);
        fprintf(fp, "%d ", cost);
      }
      fprintf(fp, "\n");
    }
    fclose(fp);
    return true;
  }

}  // namespace costmap_2d
