#pragma once
#include "MazeGrid.h"
#include "Maze.Safehouse.h"
#include <cmath>
#include <limits>
#include <queue>

inline int acousticBoundaryCost(MazePosition from,MazePosition to,
    const std::vector<MazePosition>& houses,unsigned closedDoors) noexcept
{
    for(std::size_t i=0;i<houses.size();++i)
    {
        const auto h=houses[i];
        if(insideSafehouse(from,h)==insideSafehouse(to,h)) continue;
        if((from.row!=to.row && from.col!=h.col) || (from.col!=to.col && from.row!=h.row)) return 6;
        const auto outside=insideSafehouse(from,h)?to:from;
        const int side=outside.col<h.col-2?0:outside.col>h.col+2?1:outside.row<h.row-2?2:3;
        return (closedDoors & (1u<<(i*4+side)))?5:1;
    }
    return 1;
}

// Acoustic travel can pass through foliage/walls, paying attenuation cost.
// It also finds an open route around corners rather than granting exact hearing.
inline bool acousticReachable(const MazeGrid& maze,MazePosition from,MazePosition to,
    int budget,const std::vector<MazePosition>& houses={},unsigned closedDoors=0)
{
    if(!maze.contains(from)||!maze.contains(to)||budget<0) return false;
    const int cols=maze.size().cols,start=from.row*cols+from.col,goal=to.row*cols+to.col;
    std::vector<int> costs(maze.cellCount(),std::numeric_limits<int>::max());
    using Entry=std::pair<int,int>;
    std::priority_queue<Entry,std::vector<Entry>,std::greater<Entry>> pending;
    costs[start]=0;pending.push({0,start});
    constexpr MazePosition offsets[]={{-1,0},{1,0},{0,-1},{0,1}};
    while(!pending.empty())
    {
        const auto [cost,index]=pending.top();pending.pop();
        if(cost!=costs[index]) continue;
        if(index==goal) return true;
        const MazePosition current{index/cols,index%cols};
        for(const auto offset:offsets)
        {
            const MazePosition next{current.row+offset.row,current.col+offset.col};
            if(!maze.contains(next))continue;
            const int edge=std::max(maze.at(next.row,next.col)==MazeCell::Wall?3:1,
                acousticBoundaryCost(current,next,houses,closedDoors));
            const int total=cost+edge,key=next.row*cols+next.col;
            if(total<=budget && total<costs[key]) {costs[key]=total;pending.push({total,key});}
        }
    }
    return false;
}

struct AcousticTransmission { float gain=1.f,clarity=1.f; };
inline AcousticTransmission acousticTransmission(const MazeGrid& maze,MazePosition from,MazePosition to,
    const std::vector<MazePosition>& houses={},unsigned closedDoors=0)
{
    AcousticTransmission result;
    if(!maze.contains(from)||!maze.contains(to)) return {0.f,0.f};
    int x=from.col,y=from.row;
    const int dx=std::abs(to.col-x),dy=std::abs(to.row-y),sx=x<to.col?1:-1,sy=y<to.row?1:-1;
    int error=dx-dy;
    while(x!=to.col || y!=to.row)
    {
        const MazePosition previous{y,x}; const int doubled=2*error;
        if(doubled>-dy){error-=dy;x+=sx;}
        if(doubled<dx){error+=dx;y+=sy;}
        const int boundary=acousticBoundaryCost(previous,{y,x},houses,closedDoors);
        const bool foliage=maze.at(y,x)==MazeCell::Wall;
        if(foliage || boundary>1)
        {
            result.gain*=boundary>1?.32f:.48f;
            result.clarity*=boundary>1?.30f:.48f;
        }
    }
    result.gain=std::max(.015f,result.gain);
    result.clarity=std::max(.02f,result.clarity);
    return result;
}
