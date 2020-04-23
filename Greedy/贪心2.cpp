//
//  main.cpp
//  港口调度问题【贪心2】
//
//  Created by Fever on 2019/6/12.
//  Copyright © 2019 BIT. All rights reserved.
//
#include <iostream>
#include <queue>
#include <ctime>
#define MAX 100
using namespace std;
// 入泊时间短的先进港，先吊先进港的船里用时少的货物，优先运送先放在缓冲区的货物

struct ship;
struct cargo;
int cargoN, shipN, warehouseN;
int carN, deviceN, berthN;               //输入基本数量信息
priority_queue<ship> qs;                 //货船优先队列优先出小
priority_queue<cargo> qc[MAX];           //起吊队列，时间短的优先级高
queue<int> qa;                           //运送队列优先运先来的货物
int order = 1;                           //记录入泊顺序

struct cargo                        //货物
{
    int index;                      //货物编号
    int time;                       //起吊时间
    int destinationTime;            //目标仓库时间
    friend bool operator < (cargo a, cargo b)
    {
        return a.time > b.time;     //用时少的优先级高
    }
};

struct ship     //货船
{
    int index;              //货船编号
    int amount;             //货物数量
    struct cargo c[MAX];    //货物编号
    int timeIn;             //入泊时间
    int timeOut;            //驶离时间
    int isPark = 0;         //入泊了
    int time = 0;           //正在起吊的货物所用时间
    int order;
    friend bool operator < (ship a, ship b)
    {
        return a.timeIn > b.timeIn;    //入泊用时少的优先级高
    }
};

struct warehouse //仓库
{
    int time;               //所需运输时间
};

struct car      //小车
{
    int time = 0;             //运送此货物时间
    int busy = 0;             //运货中
};

struct device   //起吊设备
{
    int busy = 0;                   //工作与否
    int time = 0;                   //吊装此货物时间
    int shipOrd = 0;                //所吊货物来自船的编号
    int dstiTime = 0;               //所吊货物运送时间
};

struct space    //缓冲区
{
    int amount = 0;         //货物数量
};

struct berth    //泊位
{
    int timeIn = 0;         //船进入的时间
    int timeOut = 0;        //船离开的时间
    int shipOrd = 0;        //所停泊的船编号
};


int isEmptyIndex(berth *b)                               //判断泊位是否空,返回首个空泊位的index
{
    for(int i = 1; i <= berthN; i++)
    {
        if(b[i].shipOrd == 0 && b[i].timeOut == 0)       //泊位已空
            return i;
    }
    return 0;                                            //泊位未空
}
int notEmptyIndex(berth *b)             //返回首个非空泊位的index
{
    for(int i = 1; i <= berthN; i++)
    {
        if(b[i].shipOrd != 0 || b[i].timeIn != 0 || b[i].timeOut != 0)
            return i;
    }
    return 0;
}
bool isAllEmpty(berth *b)               //判断泊位是否全空
{
    for(int i = 1; i <= berthN; i++)
    {
        if(b[i].shipOrd != 0 || b[i].timeIn != 0 || b[i].timeOut != 0)
            return false;
    }
    return true;
}


int notParkIndex(ship *s)               //判断船是否入泊,return index
{
    for(int i = 1; i <= shipN; i++)
    {
        if(s[i].isPark == 0)            //货船未泊
        {
            return i;
        }
    }
    return false;                       //货船已泊
}
bool isAllEmpty(ship *s)                //判断船是否空
{
    for(int i = 1; i <= shipN; i++)
    {
        if(!qc[i].empty())              //货船未空
        {
            return false;
        }
    }
    return true;                        //货船已空
}

bool isAllEmpty(car *v)                 //判断车是否全空
{
    for(int i = 1; i <= carN; i++)
    {
        if(v[i].time != 0)              //小车未空
            return false;
    }
    return true;                        //小车已空
}

bool isEmpty(device *d)                 //判断吊是否空
{
    for(int i = 1; i <= deviceN; i++)
    {
        if(d[i].time != 0)              //设备未空
            return false;
    }
    return true;                        //设备已空
}

bool isEmpty(space *a)                  //判断缓冲区是否空
{
    if(a->amount != 0)                  //设备未空
        return false;
    return true;                        //设备已空
}

int main(int argc, const char * argv[])
{
    
    clock_t start,end;
    start=clock();
    
    int totalTime = 0;              //总消耗时间
    cin >> cargoN >> shipN >> warehouseN >> carN >> deviceN >> berthN;
    
    //创建结构体记录相关信息
    ship S[shipN + 1];              //船
    warehouse W[warehouseN + 1];    //仓
    car V[carN + 1];                //车
    device D[deviceN + 1];          //塔
    space A;                        //暂
    berth B[berthN + 1];            //泊
    
    //输入仓库信息
    for(int i = 1; i <= warehouseN; i++)
    {
        cin >> W[i].time;
    }
    
    //输入货船信息
    for(int i = 1; i <= shipN; i++)
    {
        cin >> S[i].timeIn >> S[i].timeOut >> S[i].amount;
        S[i].index = i;
        qs.push(S[i]);                                  //每条船入队列
        for(int j = 1; j <= S[i].amount; j++)           //输入每条船的货物信息
        {
            cin >> S[i].c[j].time >> S[i].c[j].destinationTime;
            S[i].c[j].destinationTime = W[S[i].c[j].destinationTime].time;
            S[i].c[j].index = j;
            qc[i].push(S[i].c[j]);                      //每件货物入优先队列
        }
    }
    
    //贪心策略1解决港口调度问题
    while(totalTime >= 0)
    {
        if(isAllEmpty(S) && isAllEmpty(V) && isEmpty(D) && isEmpty(&A) && isAllEmpty(B))//货物都运送完毕&&船离开,结束计时
            break;
        totalTime++;//+1min
        
        //入泊部分
        while(isEmptyIndex(B) && notParkIndex(S) )  //有泊位空了&&有船未入泊，入船
        {
            int curBerth = isEmptyIndex(B);         //第一个空泊位
            int curShip = qs.top().index;           //入泊时间最短的船
            
            S[curShip].order = order;               //记录顺序
            order++;
            B[curBerth].shipOrd = curShip;
            B[curBerth].timeIn = S[curShip].timeIn; //计入时间
            S[curShip].isPark = 1;                  //已经停泊了
            qs.pop();                               //已经入泊的船出队列
        }
        
        //起吊部分
        for(int j = 1; j <= deviceN; j++)
        {
            int flag = 0;           //是否起吊
            if(D[j].busy == 0)      //空闲塔吊
            {
              
                int curBestB = 0, curBestS = 0, min = 999;
                for(int i = 1; i <= berthN; i++)           //遍历泊位,起吊
                {
                    if(B[i].shipOrd != 0 && S[B[i].shipOrd].amount != 0 && B[i].timeIn == 0)   //有船停着&&仍有货&&船静止允许起吊
                    {
                        if(min > S[B[i].shipOrd].order)    //找到最先入泊的货物
                        {
                            //记录下最好的货物
                            min = S[B[i].shipOrd].order;
                            curBestB = i;
                            curBestS = B[i].shipOrd;
                            flag = 1;
                        }
                    }
                }
                if(flag == 1)                   //可以起吊
                {
                    D[j].busy = 1;
                    D[j].time = qc[curBestS].top().time;
                    D[j].shipOrd = curBestS;    //来自哪艘船
                    D[j].dstiTime = qc[curBestS].top().destinationTime;
                    qc[curBestS].pop();         //出队列
                    S[curBestS].amount--;       //剩余货物--
                }
                
            }
        }
        
        //离港部分
        for(int i = 1; i <= berthN; i++)
        {
            int working = 0;                            //还在工作
            if(qc[B[i].shipOrd].empty())                //港里停的船卸空了
            {
                for(int j = 1; j <= deviceN; j++)       //确认塔吊没在吊它的货
                {
                    if(D[j].shipOrd == B[i].shipOrd)    //还在吊
                    {
                        working = 1;
                        break;
                    }
                }
                if(working == 0 && B[i].shipOrd != 0 && B[i].timeOut == 0)//吊装结束
                {
                    B[i].timeOut = S[B[i].shipOrd].timeOut;
                }
            }
        }
        
        //运送部分
        if(A.amount != 0)                               //缓冲区里有货物
        {
            for(int i = 1; i <= carN; i++)
            {
                if(V[i].time == 0 && A.amount != 0)     //车空闲 && 有货物，可以运送
                {
                    V[i].time = qa.front();             //运送时间转移给车
                    qa.pop();                           //出队列
                    A.amount--;
                    V[i].busy = 1;                      //运货中
                }
            }
        }
        
        //计时部分
        for(int i = 1; i <= berthN; i++)    //检查每个泊位的入泊与驶出时间
        {
            
            if(B[i].timeIn != 0)            //减少时间（入）
            {
                B[i].timeIn--;
            }
            if(B[i].timeOut != 0)           //减少时间（出）
            {
                B[i].timeOut--;
                if(B[i].timeOut == 0)       //离港完成
                {
                    B[i].shipOrd = 0;       //空
                }
            }
        }
        for(int i = 1; i <= deviceN; i++)   //检查每个塔吊的时间
        {
            if(D[i].time != 0 && D[i].busy) //在吊装
            {
                D[i].time--;
                if(D[i].time == 0)          //吊装完成
                {
                    D[i].busy = 0;
                    D[i].shipOrd = 0;
                    qa.push(D[i].dstiTime); //运送时间入队列
                    A.amount++;
                }
            }
        }
        for(int i = 1; i <= carN; i++)          //检查每个小车的时间
        {
            if(V[i].time != 0 && V[i].busy)     //工作中
            {
                V[i].time--;
                if(V[i].time == 0)              //运送结束
                {
                    V[i].busy = 0;              //再次空闲
                }
            }
        }
        
    }
    
    cout << "TotalTime: " << totalTime << endl;
    
    end=clock();
    printf("totile time=%f\n",(float)(end-start)*1000/CLOCKS_PER_SEC);
    return 0;
}
