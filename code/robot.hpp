#ifndef __REBOT_H__
#define __REBOT_H__

#include "config.hpp"
#include "worktable.hpp"
#include "vector.hpp"
#include "path.hpp"
#include "grid.hpp"
#include <fstream>
#include <set>
#include <map>
#include <iostream>
#include <algorithm>
#include <vector>
#include <cmath>
#include <string>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <climits>
#include <queue>

struct Robot{
    int id; // 机器人的 id
    double x; // 机器人的 x 坐标
    double y; // 机器人的 y 坐标
    int worktableId; // 机器人正在处理的工作台的 id, -1 表示没有
    int bringId; // 携带的物品的类型, 0 表示没有
    double timeCoef; // 机器人的时间系数
    double crashCoef; // 机器人的碰撞系数
    double angularSeppd; // 机器人的角速度
    double linearSpeedX; // 机器人的线速度X
    double linearSpeedY; // 机器人的线速度Y
    double direction; // 机器人的方向
    int worktableTogo; // 机器人要去的工作台的 id
    double collisionRotate; // 机器人为防止碰撞的旋转角度
    double collisionSpeed; // 机器人为防止碰撞的线速度
    int collisionSpeedTime; // 机器人为防止碰撞的调整速度的时间
    int collisionRotateTime; // 机器人为防止碰撞的时间
    Path *path; // 机器人的路径
    std::vector<Vector2D> pathPoints; // 机器人的路径点
    Robot() {
        this->id = -1;
        this->x = -1;
        this->y = -1;
        worktableId = -1;
        bringId = 0;
        timeCoef = 0;
        crashCoef = 0;
        angularSeppd = 0;
        linearSpeedX = 0;
        linearSpeedY = 0;
        direction = 0;
        collisionSpeedTime = 0;
        collisionRotateTime = 0;
        path = nullptr;
    }
    Robot(int id, double x, double y) {
        this->id = id;
        this->x = x;
        this->y = y;
        worktableId = -1;
        bringId = 0;
        timeCoef = 0;
        crashCoef = 0;
        angularSeppd = 0;
        linearSpeedX = 0;
        linearSpeedY = 0;
        direction = 0;
        collisionSpeedTime = 0;
        collisionRotateTime = 0;
        path = nullptr;
    }
    void checkCanBuy() {
        if (bringId != 0) {
            canBuy[bringId]--;
        }
    }
    void outputTest() {
        TESTOUTPUT(fout << "Robot id: " << id << std::endl;)
        // TESTOUTPUT(fout << "x: " << x << std::endl;)
        // TESTOUTPUT(fout << "y: " << y << std::endl;)
        // TESTOUTPUT(fout << "worktableId: " << worktableId << std::endl;)
        // TESTOUTPUT(fout << "bringId: " << bringId << std::endl;)
        // TESTOUTPUT(fout << "timeCoef: " << timeCoef << std::endl;)
        // TESTOUTPUT(fout << "crashCoef: " << crashCoef << std::endl;)
        // TESTOUTPUT(fout << "angularSeppd: " << angularSeppd << std::endl;)
        // TESTOUTPUT(fout << "linearSpeedX: " << linearSpeedX << std::endl;)
        // TESTOUTPUT(fout << "linearSpeedY: " << linearSpeedY << std::endl;)
        // TESTOUTPUT(fout << "direction: " << direction << std::endl;)
        // TESTOUTPUT(fout << "worktableTogo: " << worktableTogo << std::endl;)
        // TESTOUTPUT(fout << "collisionRotate: " << collisionRotate << std::endl;)
        // TESTOUTPUT(fout << "collisionSpeed: " << collisionSpeed << std::endl;)
        TESTOUTPUT(fout << "speed" << Vector2D(linearSpeedX, linearSpeedY).length() << std::endl;)
    }
    // 卖材料的函数
    void Sell() {
        if (bringId == 0) return;// 机器人没有携带原材料
        if (path == nullptr) return;
        // 机器人正在携带原材料
        if (worktableId != path->sellWorktableId) return; // 机器人不在预想工作台附近
        // 机器人正在工作台附近
        if (worktables[worktableId].inputId[bringId] == 0 
            && sellSet.find(std::make_pair(bringId, worktables[worktableId].type)) != sellSet.end()) {
            // 该工作台的此原料不满,且支持卖出
            TESTOUTPUT(fout << "sell " << id << std::endl;)
            printf("sell %d\n", id);
            // 统计损失
            lossCollMoney += sellMoneyMap[worktables[worktableId].type] * (1 - crashCoef);
            if (crashCoef < 1) {
                TESTOUTPUT(fout << "有碰撞损失: " << crashCoef << std::endl;)
            }
            lossTimeMoney += sellMoneyMap[worktables[worktableId].type] * (1 - timeCoef);
            // 工作台这一位设置成有东西了
            worktables[worktableId].inputId[bringId] = 1;
            {
                /**
                 * 如果没有生产的情况下, 该工作台的所有原料都满了, 则清空该工作台的原料
                */
                bool full = true;
                for (int item = 1; item <= MAX_Item_Type_Num; item++) {
                    if (sellSet.find(std::make_pair(item, worktables[worktableId].type)) != sellSet.end()) {
                        if (worktables[worktableId].inputId[item] == 0) {
                            full = false;
                            break;
                        }
                    }
                }
                if (full && worktables[worktableId].remainTime == -1) {
                    for (int item = 1; item <= MAX_Item_Type_Num; item++) {
                        worktables[worktableId].inputId[item] = 0;
                    }
                }
            }
            worktables[worktableId].someWillSell[bringId]--;
            bringId = 0;
            path = nullptr;
            worktableTogo = -1;
        }
    }
    // 买商品的函数
    void Buy() {
        if (bringId != 0) return;// 机器人已经携带原材料
        if (path == nullptr) return;
        // 机器人没有携带原材料
        if (worktableId != path->buyWorktableId) return; // 机器人不在预想工作台附近
        // 机器人正在工作台附近
        if (worktables[worktableId].output == true && money >= buyMoneyMap[createMap[worktables[worktableId].type]]) {
            // 如果买了卖不出去
            TESTOUTPUT(fout << "buy " << id << std::endl;)
            printf("buy %d\n", id);
            bringId = createMap[worktables[worktableId].type];
            money -= buyMoneyMap[createMap[worktables[worktableId].type]];
            worktables[worktableId].someWillBuy--;
            worktables[worktableId].output = false;
            if (worktables[worktableId].remainTime == 0) {
                worktables[worktableId].output = true;
            }
            worktableTogo = path->sellWorktableId;
            pathPoints = movePath();
        }
    }


    void Move() {
        Vector2D go;
        if (worktableTogo == -1) {
            pathPoints.push_back(Vector2D(worktables[1].x, worktables[1].y));
        }
        while ((Vector2D(x, y) - pathPoints[0]).length() < 0.4) {
            pathPoints.erase(pathPoints.begin());
        }
        TESTOUTPUT(fout << "from" << "(" << x << ", " << y << ")" << "to" << "(" << pathPoints[0].x << ", " << pathPoints[0].y << ")" << std::endl;)
        std::vector<double> vec1 = {1, 0};
        std::vector<double> vec2 = {pathPoints[0].x - x, pathPoints[0].y - y};
        double cosAns = (vec1[0] * vec2[0] + vec1[1] * vec2[1]) / (sqrt(vec1[0] * vec1[0] + vec1[1] * vec1[1]) * sqrt(vec2[0] * vec2[0] + vec2[1] * vec2[1]));
        double angle = acos(cosAns);
        // 通过叉乘判断方向
        // > 0 逆时针 在上面
        // < 0 顺时针 在下面
        if (vec1[0] * vec2[1] - vec1[1] * vec2[0] < 0) {
            angle = -angle;
        }
        /*
            决定一下转向的大小
        */
        double rotate = 0;
        if (angle > direction) {
            if (std::abs(angle - direction) < M_PI) { // 如果角度差小于180°
                rotate = std::abs(angle - direction); // 就直接逆时针转过去
            } else { // 如果角度差大于180°
                rotate = -(M_PI*2-std::abs(angle - direction)); // 就顺时针转过去
            }
        } else { 
            if (std::abs(angle - direction) < M_PI) { // 如果角度差小于180°
                rotate = -(std::abs(angle - direction)); // 就直接顺时针转过去
            } else { // 如果角度差大于180°
                rotate = M_PI*2-std::abs(angle - direction); // 就逆时针转过去
            }
        }

        double absRotate = std::abs(rotate);
        double length = sqrt(vec2[0] * vec2[0] + vec2[1] * vec2[1]);
        /*
            决定一下速度
        */
        double speed = 0;
        // TESTOUTPUT(fout << "absRotate: " << absRotate << " length =" << length << " asin=" << asin(0.4 / length) << std::endl;)
        // 由于判定范围是 0.4m,所以如果度数满足这个条件, 就直接冲过去
        // if (0.4 / length > 1) {
        //     speed = 6;
        // } else
        if (absRotate < asin(0.4 / length)) {
            speed = 6;
        } else if (absRotate < M_PI / 4) {
            // 如果度数大于x小于45° 
            // 如果转向时间的路程不会更长,就可以走
            if (absRotate > 0.0001)
                speed = length * 50 / (absRotate / 3.6 + 1);
            if (speed > 6) {
                speed = 6;
            }
        } else if (absRotate < M_PI / 2) {
            // 如果度数大于45°小于 90°
            speed = 0;
        } else {
            // 如果度数大于90°, 就先倒退转过去
            speed = -2;
        }
        if (collisionSpeedTime > 0) {
            collisionSpeedTime--;
            speed = collisionSpeed;
        }
        TESTOUTPUT(fout << "forward " << id << " " << speed << std::endl;)
        printf("forward %d %lf\n", id, speed);
        /*
            由于转向决定的是速度,所以 1s 最多转3.6°
            如果转向大于3.6°, 就直接最大速度转向
            如果转向小于3.6°, 就按照比例转向
        */
        if (absRotate > 0.0001){
            if (absRotate < M_PI / 25) {
                double Ratio = absRotate / (M_PI / 25); 
                rotate = rotate / absRotate * M_PI * Ratio;
            } else {
                rotate = rotate / absRotate * M_PI;
            }
        }
        if (collisionRotateTime > 0) {
            collisionRotateTime--;
            rotate = collisionRotate;
        }
        TESTOUTPUT(fout << "rotate " << id << " " << rotate << std::endl;)
        printf("rotate %d %lf\n", id, rotate);
    }
    // 如果没地方卖,就销毁
    int Destroy() {
        /**
         * 特殊情况，有个工作台快做完了，而且材料全了（没啥用，暂时弃用）
        */
        for (auto i : worktables) {
            if (i.remainTime > 0 && sellSet.find(std::make_pair(bringId, i.type)) != sellSet.end()) {
                int all = 0;
                int have = 0;
                for (int item = 1; item <= MAX_Item_Type_Num; item++) {
                    if (sellSet.find(std::make_pair(item, i.type)) != sellSet.end()) {
                        all++;
                        if (i.inputId[item] > 0) {
                            have++;
                        }
                    }
                }
                if (have == all) {
                    return i.id;
                }
            }
        }
        TESTOUTPUT(fout << "destroy " << id << std::endl;)
        printf("destroy %d\n", id);
        return -1;
    }
    double getMinGoToTime(double x1, double y1, double x2, double y2) {
        double length = (Vector2D(x1, y1) - Vector2D(x2, y2)).length();
        return length / 0.12 + 25;
    }
    void FindAPath() {
        std::vector<Path *> paths;
        std::vector<Path *> paths4567;
        for (auto & buy : worktables) {
            /**
             * 有产物, 没人预约
             * 有产物, 有人预约买,但是第二个在做或者做完阻塞了
             * 没产物, 没人预约买,在做了
            */
            if (buy.id == -1) break;
            if ((buy.output == true && buy.someWillBuy == 0)
                || (buy.output == true && buy.someWillBuy == 1 && buy.remainTime != -1)
                || (buy.output == false && buy.someWillBuy == 0 && buy.remainTime != -1)
                || (buy.type < 4)
            ) {} else continue;
            // 等待生产的时间
            double waitBuyTime = 0; 
            if ( (buy.someWillBuy == 1) || (buy.output == false && buy.someWillBuy == 0) ) waitBuyTime = buy.remainTime;
            // 路程时间消耗
            double goBuyTime = getMinGoToTime(x, y, buy.x, buy.y);
            // 如果等待时间比路程时间长,就不用买了
            if (goBuyTime < waitBuyTime) continue;
            // 购买的产品
            int productId = createMap[buy.type];
            for (auto & sell : worktables) {
                if (sell.id == -1) break;
                // 确保这个工作台支持买,而且输入口是空的
                if (sellSet.find(std::make_pair(productId, sell.type)) == sellSet.end() || sell.inputId[productId] == 1) continue;
                // 确保不是墙角
                if (sell.isNearCorner) continue;
                /**
                 * 确保没人预约卖
                 * 或者类型是8 || 9
                */
                if (sell.someWillSell[productId] == 0 || sell.type == 8 || sell.type == 9) {} else continue;
                // 时间消耗
                double goSellTime = getMinGoToTime(buy.x, buy.y, sell.x, sell.y);
                double sumTime = std::max(goBuyTime, waitBuyTime) + goSellTime;
                if (sumTime + 30 + nowTime > MAX_TIME) continue;
                // 时间损失
                double timeLoss;
                if (sumTime >= 9000) {
                    timeLoss = 0.8;
                } else {
                    timeLoss = 0.8 + (1-0.8) * (1 - sqrt(1 - (1 - sumTime / 9000) * (1 - sumTime / 9000)));
                }
                // 卖出产品赚取的钱
                double earnMoney = sellMoneyMap[productId] * timeLoss - buyMoneyMap[productId];
                // 尽量不卖给 9
                if (sell.type == 9) earnMoney = earnMoney * 0.6;
                earnMoney *= buy.near7;
                earnMoney *= sell.near7;
                if (sell.waitPriority == 5) {
                    earnMoney *= 1.2;
                }
                // 有资源缺口 即卖工作台的类型对应的产品(type 相同)有缺口 就促进生产
                if (canBuy[sell.type] > 0 && (sell.type == 4 || sell.type == 5 || sell.type == 6) && sell.near7 != 1) {
                    // TESTOUTPUT(fout << "canBuy " << sell.type << std::endl;)
                    earnMoney *= 1.2;
                }
                Path * path = new Path(buy.id, sell.id, id, earnMoney, sumTime);
                if ((productId == 4 || productId == 5 || productId == 6 || productId == 7) && ((buy.remainTime == 0 && buy.someWillBuy == 0) || (buy.remainTime < goBuyTime && buy.output == true && buy.someWillBuy == 0))) {
                    paths4567.push_back(path);
                }else {
                    paths.push_back(path);
                }
            }
        }
        // 理论上只有快结束才出现
        if (paths.size() == 0 && paths4567.size() == 0) {
            TESTOUTPUT(fout << "error" << std::endl;)
            worktableTogo = -1;
            return;
        }
        std::sort(paths.begin(), paths.end(), [](Path * a, Path * b) {
            return a->parameters > b->parameters;
        });
        std::sort(paths4567.begin(), paths4567.end(), [](Path * a, Path * b) {
            return a->parameters > b->parameters;
        });
        if (paths4567.size() > 0 && (paths.size() == 0 || paths4567[0]->parameters > paths[0]->parameters * 0.95)) {
            path = paths4567[0];
        } else {
            path = paths[0];
        }
        TESTOUTPUT(fout << "robot" << id << " find path " << path->buyWorktableId << " " << path->sellWorktableId << std::endl;)
        worktableTogo = path->buyWorktableId;
        pathPoints = movePath();
        worktables[path->buyWorktableId].someWillBuy++;
        worktables[path->sellWorktableId].someWillSell[createMap[worktables[path->buyWorktableId].type]]++;
    }
    double point_to_segment_distance(Vector2D begin, Vector2D end, Vector2D obstacle) {
        Vector2D begin_to_end = end-begin;
        Vector2D begin_to_obstacle = obstacle - begin;
        Vector2D end_to_obstacle = obstacle - end;
        if (begin_to_end * begin_to_obstacle <= 0) return (begin-obstacle).length();
        if (begin_to_end * end_to_obstacle >= 0) return (end-obstacle).length();
        return fabs(begin_to_end ^ begin_to_obstacle) / (begin-end).length();
    }
    std::vector<Vector2D> fixpath(std::vector<Vector2D> path) {
        // TESTOUTPUT(fout << path.size() << std::endl;)
        std::vector<Vector2D> ret;
        auto begin = path.begin();
        // 不断延迟线段的终点
        // 如果碰撞了,就不加这个点, 以最后一个点开始继续这个过程
        // 如果没有碰撞,就加上这个点的碰撞点
        while (begin != path.end()) {
            // TESTOUTPUT(fout << "线段从 " << begin->x << "," << begin->y << "开始" << std::endl;)
            auto end = begin;
            auto obstacles = grids[*begin]->obstacles;
            while (end != path.end()) {
                end++;
                if (end == path.end()) break;
                bool flag = false;
                // TESTOUTPUT(fout << "测试到 " << end->x << "," << end->y << "是否碰撞" << std::endl;)
                for (auto & obstacle : obstacles) {
                    // 计算obstacle到 begin->end这条线段的距离
                    double distance = point_to_segment_distance(*begin, *end, obstacle);
                    if (distance < 0.53) {// 碰撞了
                        // TESTOUTPUT(fout << "碰撞点" << obstacle.x << "," << obstacle.y << std::endl;)
                        flag = true;
                        break;
                    }
                }
                if (flag) {
                    // TESTOUTPUT(fout << "碰撞了" << std::endl;)
                    break;
                }
                for (auto & obstacle : grids[*end]->obstacles) {
                    obstacles.push_back(obstacle);
                }
            }
            // end--;
            ret.push_back(*begin);
            begin = end;
        }
        ret.push_back(path.back());
        for (auto & item : ret) {
            TESTOUTPUT(fout << "(" << item.x << "," << item.y << ")" << "->";)
        }
        TESTOUTPUT(fout << std::endl;)
        return ret;
    }
    /**
     * 计算路径
     * 计算从一个坐标移动到另一个坐标的路径
     * 通过 BFS 实现
     * 返回值应该是一个n个点的坐标的数组
    */
    std::vector<Vector2D> movePath(){
        Vector2D to(worktables[worktableTogo].x, worktables[worktableTogo].y);
        std::vector<Vector2D> path;
        std::map<Vector2D, Vector2D> fromWhere;
        std::queue<Vector2D> q;
        double x = this->x;
        double y = this->y;
        x = int(x / 0.5) * 0.5 + 0.25;
        y = int(y / 0.5) * 0.5 + 0.25;
        q.push(Vector2D(x, y));
        fromWhere.insert(std::make_pair(Vector2D(x, y), Vector2D(x, y)));
        int findItme = 0;
        bool find = false;
        while (!q.empty() && find == false) {
            findItme++;
            Vector2D now = q.front();
            q.pop();
            std::vector<std::pair<double, double>> adds = {{0, 0.5}, {0.5, 0}, {0, -0.5}, {-0.5, 0}, {0.5, 0.5}, {-0.5, 0.5}, {0.5, -0.5}, {-0.5, -0.5}, {0, 0}};
            for (auto &add : adds) {
                Vector2D index = now + Vector2D(add.first, add.second);
                if (index.x <= 0.25 || index.x >= 49.75 || index.y <= 0.25 || index.y >= 49.75) continue;
                if (fromWhere.find(index) != fromWhere.end()) continue;
                if (grids[index]->type == 1) continue;
                bool flag = false;
                if (!(index == to))for (auto & item : grids[index]->obstacles) {
                    if ((index - item).length() < (bringId == 0 ? 0.45 : 0.53)) {
                        flag = true;
                        break;
                    }
                }
                if (flag) continue;
                fromWhere.insert(std::make_pair(index, now));
                q.push(index);
                if (now == to){
                    find = true;
                }
            }
        }
        // TESTOUTPUT(fout << "find " << findItme << std::endl;)
        while ( 1 ) {
            path.push_back(to);
            if (to == fromWhere[to]) break;
            to = fromWhere[to];
        }
        std::reverse(path.begin(), path.end());
        // for (auto & item : path) {
        //     TESTOUTPUT(fout << "(" << item.x << "," << item.y << ")" << "->";)
        // }
        // TESTOUTPUT(fout << std::endl;)
        // path = fixpath(path);
        return path;
    }
    // 机器人具体的行为
    void action(){
        Sell();
        if (path == nullptr) FindAPath();
        Buy();
        TESTOUTPUT(fout << "robot" << id << " want-go " << worktableTogo << " type=" << worktables[worktableTogo].type << std::endl;)
    }
    void checkWall() {
        if (worktableTogo == -1) return;
        bool wallnear = false;
        double toWallX = std::min(worktables[worktableTogo].y - 0.53 - 0.12, 50 - 0.53 - 0.12 -worktables[worktableTogo].y);
        double toWallY = std::min(worktables[worktableTogo].x - 0.53 - 0.12, 50 - 0.53 - 0.12 -worktables[worktableTogo].x);
        // 刹车最常的距离 1.86
        if (toWallX < 0.02 * 3 * 31 || toWallY < 0.02 * 3 * 31) {
            wallnear = true;
        }
        if (wallnear == false) return;
        if (worktables[worktableTogo].type == 9 || worktables[worktableTogo].type == 8) {
            return;
        }
        if (bringId > 0 && worktables[worktableTogo].output == false) {
            return;
        }
        double speed = Vector2D(linearSpeedX, linearSpeedY).length();
        double length = 0;
        int costTime = 0;
        while (speed > 0) {
            length += speed * 0.02;
            costTime++;
            speed -= 0.3;
        }
        // double radii = bringId == 0 ? 0.45 : 0.53;
        // double acceleration = 250 / (20 * M_PI * radii * radii * 50);
        // double length = speed * speed / (2 * acceleration);
        toWallX = std::min(y - 0.53 - 0.12, 50 - 0.53 - 0.12 -y);
        toWallY = std::min(x - 0.53 - 0.12, 50 - 0.53 - 0.12 -x);
        TESTOUTPUT(fout << "robot" << id << " length=" << length << " toWallX=" << toWallX << " toWallY=" << toWallY << std::endl;)
        if (toWallX < std::abs(length * sin(direction)) || toWallY < std::abs(length * cos(direction))) {
            TESTOUTPUT(fout << "robot" << id << " 有撞墙风险 " << std::endl;)
            // Vector2D normal(0, 0);
            // if (toWallX < length * sin(direction) && toWallX == y-0.53-0.1) {
            //     normal = Vector2D(0, -1);
            // } else if (toWallX < length * sin(direction) && toWallX == 50-0.53-0.1-y) {
            //     normal = Vector2D(0, 1);
            // } else if (toWallY < length * cos(direction) && toWallY == x-0.53-0.1) {
            //     normal = Vector2D(-1, 0);
            // } else if (toWallY < length * cos(direction) && toWallY == 50-0.53-0.1-x) {
            //     normal = Vector2D(1, 0);
            // }
            // collisionSpeed = -2;
            // collisionRotate = normal^Vector2D(cos(direction), sin(direction)) * M_PI;
            collisionSpeed = -2;
            collisionSpeedTime = costTime - 1;
            // if (bringId != 0)
            // collisionRotate = M_PI;
        }
    }
};

Robot robots[MAX_Robot_Num];

double solveChangeSpeed(double speed, double diff) {
    speed += diff;
    if (speed > 6) {
        speed = 6;
    }
    if (speed < -2) {
        speed = -2;
    }
    return speed;
}

void DetecteCollision(int robot1, int robot2) {
    if (robots[robot1].worktableTogo == -1 || robots[robot2].worktableTogo == -1) return;
    // robot1 robot2 的坐标
    Vector2D robot1Pos = Vector2D(robots[robot1].x, robots[robot1].y);
    Vector2D robot2Pos = Vector2D(robots[robot2].x, robots[robot2].y);
    // robot1 robot2 的半径
    double robot1Radii = robots[robot1].bringId == 0 ? 0.451 : 0.531;
    double robot2Radii = robots[robot2].bringId == 0 ? 0.451 : 0.531;
    // 距离太远
    if ((robot1Pos-robot2Pos).length() > futureTime * 0.12 * 2 + robot1Radii + robot2Radii) {
        return;
    }
    bool isCollision = false;
    int collisionTime = 0;
    for (int i = 0; i <= futureTime; i++) {
        auto robot1PosTemp = robot1Pos + Vector2D(robots[robot1].linearSpeedX * 0.02 * i, robots[robot1].linearSpeedY * 0.02 * i);
        auto robot2PosTemp = robot2Pos + Vector2D(robots[robot2].linearSpeedX * 0.02 * i, robots[robot2].linearSpeedY * 0.02 * i);
        if ((robot1PosTemp-robot2PosTemp).length() <= robot1Radii + robot2Radii + 0.24) {
            isCollision = true;
            collisionTime = i;
            break;
        }
    }
    if (!isCollision) {
        return;
    }
    TESTOUTPUT(fout << "robot" << robot1 << " and robot" << robot2 << " 检测碰撞" << std::endl;)

    double angle = robots[robot1].direction - robots[robot2].direction;
    angle = std::abs(angle);
    if (angle > M_PI) {
        angle = 2 * M_PI - angle;
    }
    TESTOUTPUT(fout << "碰撞角度" << angle << std::endl;)

    if (angle > M_PI * 150 / 180) { // 135~180  ! 或许都是一个方向会比较好用
        if (robots[robot1].collisionRotateTime > 0) {
            return;
        }
        TESTOUTPUT(fout << "collision need rotate" << std::endl;)
        double status1 = Vector2D(cos(robots[robot1].direction), sin(robots[robot1].direction))^Vector2D(robots[robot2].x - robots[robot1].x, robots[robot2].y - robots[robot1].y);
        status1 = status1 > 0 ? 1 : -1;
        double status2 = Vector2D(cos(robots[robot2].direction), sin(robots[robot2].direction))^Vector2D(robots[robot1].x - robots[robot2].x, robots[robot1].y - robots[robot2].y);
        status2 = status2 > 0 ? 1 : -1;
        // 叉积 > 0 逆时针到达对方. < 0 顺时针到达对方
        robots[robot1].collisionRotate = -status1 * M_PI;
        robots[robot2].collisionRotate = -status2 * M_PI;
        robots[robot1].collisionSpeed = 6;
        robots[robot2].collisionSpeed = 6;
        robots[robot1].collisionSpeedTime = 1;
        robots[robot2].collisionSpeedTime = 1;
        robots[robot1].collisionRotateTime = 1;
        robots[robot2].collisionRotateTime = 1;
        if (angle > M_PI * 175 / 180) {
            TESTOUTPUT(fout << "碰撞大角度" << std::endl;)
            robots[robot1].collisionRotateTime = 8;
            robots[robot2].collisionRotateTime = 8;
        }
        if ((robot1Pos-robot2Pos).length() - robot1Radii - robot2Radii - 0.12 < 0) { // 12 最大角速度转向时间 16 角速度改变最大时间
            TESTOUTPUT(fout << "collision need go back" << std::endl;)
            // 距离比较近的情况, 考虑到预测范围没预测到,或者发生了被赢拽回来了的情况
            robots[robot1].collisionRotate = M_PI;
            robots[robot2].collisionRotate = M_PI;
            robots[robot1].collisionSpeed = 6;
            robots[robot2].collisionSpeed = 6;
            robots[robot1].collisionSpeedTime = 1;
            robots[robot2].collisionSpeedTime = 1;
            robots[robot1].collisionRotateTime = 1;
            robots[robot2].collisionRotateTime = 1;
            return;
        }
        return;
    }

    double speed1 = Vector2D(robots[robot1].linearSpeedX, robots[robot1].linearSpeedY).length();
    if (sin(robots[robot1].direction) * robots[robot1].linearSpeedY < 0 || cos(robots[robot1].direction) * robots[robot1].linearSpeedX < 0) {
        speed1 = -speed1;
        TESTOUTPUT(fout << "反方向 robot" << robot1 << " speed1 = " << speed1 << std::endl;)
    }
    double speed2 = Vector2D(robots[robot2].linearSpeedX, robots[robot2].linearSpeedY).length();
    if (sin(robots[robot2].direction) * robots[robot2].linearSpeedY < 0 || cos(robots[robot2].direction) * robots[robot2].linearSpeedX < 0) {
        speed2 = -speed2;
        TESTOUTPUT(fout << "反方向 robot" << robot2 << " speed2 = " << speed2 << std::endl;)
    }
    // 按照0.3的速度改变为一个单位, 时间跨度是futureTime次, 改变单位的次数是 - future ~ future
    // 根据牵引力 大小 密度 算出来的加速度
    double acceleration1 = 250 / (20 * M_PI * robot1Radii * robot1Radii * 50);
    double acceleration2 = 250 / (20 * M_PI * robot2Radii * robot2Radii * 50);
    for (int accelerationTime1 = futureTime; accelerationTime1 >= -futureTime; accelerationTime1--) {
        for (int accelerationTime2 = futureTime; accelerationTime2 >= -futureTime; accelerationTime2--) {
            // 枚举每个机器人速度改变的帧率数量
            Vector2D robot1PosTemp = robot1Pos;
            Vector2D robot2PosTemp = robot2Pos;
            // 下一帧位置 i = 1 第一帧不论速度怎么设, 仍然维持之前的速度
            // 下一帧移动,然后才改变速度
            robot1PosTemp = robot1PosTemp + Vector2D(robots[robot1].linearSpeedX * 0.02, robots[robot1].linearSpeedY * 0.02);
            robot2PosTemp = robot2PosTemp + Vector2D(robots[robot2].linearSpeedX * 0.02, robots[robot2].linearSpeedY * 0.02);
            // 从第二帧开始检测
            bool isCollision = false;
            // 根据加速度和次数计算出每次的该变量
            double changeSpeed1 = 0;
            double changeSpeed2 = 0;
            if (accelerationTime1 != 0) changeSpeed1 = accelerationTime1 / std::abs(accelerationTime1) * acceleration1;
            if (accelerationTime2 != 0) changeSpeed2 = accelerationTime2 / std::abs(accelerationTime2) * acceleration2;
            // 计算出每个机器人需要改变多少次
            int changeTime1 = std::abs(accelerationTime1);
            int changeTime2 = std::abs(accelerationTime2);
            // 目前使用的速度
            double testSpeed1 = speed1;
            double testSpeed2 = speed2;
            if (changeTime1-- > 0) testSpeed1 = solveChangeSpeed(testSpeed1, changeSpeed1);
            if (changeTime2-- > 0) testSpeed2 = solveChangeSpeed(testSpeed2, changeSpeed2);
            for (int i = 2; i <= futureTime; i++) {
                robot1PosTemp = robot1PosTemp + Vector2D(testSpeed1 * cos(robots[robot1].direction) * 0.02, testSpeed1 * sin (robots[robot1].direction) * 0.02);
                robot2PosTemp = robot2PosTemp + Vector2D(testSpeed2 * cos(robots[robot2].direction) * 0.02 , testSpeed2 * sin (robots[robot2].direction) * 0.02);
                if ((robot1PosTemp-robot2PosTemp).length() <= robot1Radii + robot2Radii + 0.24) {
                    isCollision = true;
                    break;
                }
                if (changeTime1-- > 0) testSpeed1 = solveChangeSpeed(testSpeed1, changeSpeed1);
                if (changeTime2-- > 0) testSpeed2 = solveChangeSpeed(testSpeed2, changeSpeed2);
            }
            if (std::abs(testSpeed1) < 2 && std::abs(testSpeed2) < 2) {
                continue;
            }
            if (!isCollision) {
                TESTOUTPUT(
                    fout << "robot" << robot1 << " 速度改变" << accelerationTime1 << " 帧" << std::endl;
                    fout << "robot" << robot2 << " 速度改变" << accelerationTime2 << " 帧" << std::endl;
                    fout << "robot" << robot1 << " 速度从" << speed1 << "->" << testSpeed1 << std::endl;
                    fout << "robot" << robot2 << " 速度从" << speed2 << "->" << testSpeed2 << std::endl;
                )
                if (accelerationTime1 > 0) {
                    robots[robot1].collisionSpeed = 6;
                } else if (accelerationTime1 < 0) {
                    robots[robot1].collisionSpeed = -2;
                } else {
                    robots[robot1].collisionSpeed = speed1;
                }
                if (accelerationTime2 > 0) {
                    robots[robot2].collisionSpeed = 6;
                } else if (accelerationTime2 < 0) {
                    robots[robot2].collisionSpeed = -2;
                } else {
                    robots[robot2].collisionSpeed = speed2;
                }
                robots[robot1].collisionSpeedTime = collisionTime - 1;
                robots[robot2].collisionSpeedTime = collisionTime - 1;
                robots[robot1].collisionRotate = 0;
                robots[robot2].collisionRotate = 0;
                robots[robot1].collisionRotateTime = collisionTime - 6;
                robots[robot2].collisionRotateTime = collisionTime - 6;
                return;
            }
        }
    }
    TESTOUTPUT(fout << "could not find a solution" << std::endl;)
    double status1 = Vector2D(cos(robots[robot1].direction), sin(robots[robot1].direction))^Vector2D(robots[robot2].x - robots[robot1].x, robots[robot2].y - robots[robot1].y);
    status1 = status1 > 0 ? 1 : -1;
    double status2 = Vector2D(cos(robots[robot2].direction), sin(robots[robot2].direction))^Vector2D(robots[robot1].x - robots[robot2].x, robots[robot1].y - robots[robot2].y);
    status2 = status2 > 0 ? 1 : -1;
    // 叉积 > 0 逆时针到达对方. < 0 顺时针到达对方
    robots[robot1].collisionRotate = -status1 * M_PI;
    robots[robot2].collisionRotate = status2 * M_PI;
    robots[robot1].collisionSpeed = 6;
    robots[robot2].collisionSpeed = -2;
    robots[robot1].collisionSpeedTime = 1;
    robots[robot2].collisionSpeedTime = 1;
    robots[robot1].collisionRotateTime = 1;
    robots[robot2].collisionRotateTime = 1;
    return;
}
#endif