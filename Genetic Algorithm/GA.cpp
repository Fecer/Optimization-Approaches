//
//  main.cpp
//  港口调度问题【元启发】
//
//  Created by Fever on 2019/6/27.
//  Copyright © 2019 BIT. All rights reserved.
//
/****************************************/
//功能：使用遗传算法生成货物序列
//遗传算法细节：
//编码：优先级编码
//选择：轮盘赌法
//交叉：单点交叉，交叉概率固定
//变异：平均随机变异，变异概率固定
/****************************************/
#include <iostream>
#include <cstdlib>
#include <algorithm>
#include <random>
#include <queue>
#include <array>
#include <cstring>
#include <ctime>
#define MAX 100
using namespace std;

//参数表
const int populationSize = 1000;    //种群规模
const int chromosomeLen = 23;     //染色体长度
double crossoverRate = 0.5;    //交叉率
double mutationRate = 0.001;   //变异率
int generationNum = 200;     //代数

struct Chromosome
{
    //    int prioritySequence[chromosomeLen];    //货物的优先级队列: 0 - 3是船的顺序,4-22是货物优先级顺序
    array<int, chromosomeLen> prioritySequence;
    double fitValue = 0;   //适应值（总用时的倒数的1000倍）
    double fitRate;        //适应值百分比
    double fitAccumulate;  //累计概率
    int time;              //总用时
};

/****************************************/
//函数声明
void init(Chromosome (&c)[populationSize]);         //初始化
double calFit(Chromosome (&c)[populationSize], int idx);        //计算适应度
void update(Chromosome (&c)[populationSize]);       //更新种群属性
void select(Chromosome (&c)[populationSize], Chromosome (&nextc)[populationSize], Chromosome &best_individual);       //轮盘赌选择
void crossover(Chromosome (&nextc)[populationSize]);//交叉
void mutation(Chromosome (&nextc)[populationSize]); //突变
/****************************************/


/****************************************/
//结构框架（来自贪心）
struct ship;
struct cargo;
int cargoN = 19, shipN = 4, warehouseN = 3;                    //输入基本数量信息
int carN = 2, deviceN = 2, berthN = 2;                        //输入基本数量信息
priority_queue<ship> qs;                          //货船优先队列优先出小
priority_queue<cargo> qc[MAX];                    //起吊优先队列优先出小
priority_queue<int> qa;                           //运送优先队列优先出小
priority_queue<cargo> C;                          //新队列

struct cargo                                      //货物
{
    int index;                                    //货物编号
    int time;                                     //起吊时间
    int destinationTime;                          //目标仓库时间
    int priority;                                 //优先级
    int shipIndex;
    friend bool operator < (cargo a, cargo b)
    {
        return a.priority > b.priority;                   //优先级小的优先级高
    }
};

struct ship                                         //货船
{
    int index;                                      //货船编号
    int amount;                                     //货物数量
    struct cargo c[MAX];                            //货物编号
    int timeIn;                                     //入泊时间
    int timeOut;                                    //驶离时间
    int isPark = 0;                                 //入泊了
    int time = 0;                                   //正在起吊的货物所用时间
    int priority = 0;                               //优先级
    friend bool operator < (ship a, ship b)
    {
        return a.priority > b.priority;
    }
};

struct warehouse   //仓库
{
    int time;      //所需运输时间
};

struct car          //小车
{
    int time = 0;   //运送此货物时间
    int busy = 0;   //运货中
};

struct device                       //起吊设备
{
    int busy = 0;                   //工作与否
    int time = 0;                   //吊装此货物时间
    int shipOrd = 0;                //所吊货物来自船的编号
    int dstiTime = 0;               //所吊货物运送时间
};

struct space                //缓冲区
{
    int amount = 0;         //货物数量
};

struct berth                //泊位
{
    int timeIn = 0;         //船进入的时间
    int timeOut = 0;        //船离开的时间
    int shipOrd = 0;        //所停泊的船编号
};


ship S[4 + 1];              //船
warehouse W[3 + 1];         //仓
car V[2 + 1];               //车
device D[2 + 1];            //塔
space A;                    //暂
berth B[2 + 1];             //泊


int isEmptyIndex(berth *b)                               //判断泊位是否空,返回首个空泊位的index
{
    for(int i = 1; i <= berthN; i++)
    {
        if(b[i].shipOrd == 0 && b[i].timeOut == 0)       //泊位已空
            return i;
    }
    return 0;                                            //泊位未空
}
int notEmptyIndex(berth *b)                              //返回首个非空泊位的index
{
    for(int i = 1; i <= berthN; i++)
    {
        if(b[i].shipOrd != 0 || b[i].timeIn != 0 || b[i].timeOut != 0)
            return i;
    }
    return 0;
}
bool isAllEmpty(berth *b)                               //判断泊位是否全空
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
        if(s[i].amount)          //货船未空
        {
            return false;
        }
    }
    return true;                    //货船已空
}

bool isAllEmpty(car *v)             //判断车是否全空
{
    for(int i = 1; i <= carN; i++)
    {
        if(v[i].time != 0)          //小车未空
            return false;
    }
    return true;                    //小车已空
}

bool isEmpty(device *d)             //判断吊是否空
{
    for(int i = 1; i <= deviceN; i++)
    {
        if(d[i].time != 0)          //设备未空
            return false;
    }
    return true;                    //设备已空
}

bool isEmpty(space *a)              //判断缓冲区是否空
{
    if(a->amount != 0)              //设备未空
        return false;
    return true;                    //设备已空
}


/****************************************/

int main(int argc, const char * argv[])
{
    clock_t start,end;//计时
    start=clock();
    
    Chromosome c[populationSize];
    
    for(int i = 1; i <= warehouseN; i++)
    {
        cin >> W[i].time;
    }
    
    //输入货船信息
    for(int i = 1; i <= shipN; i++)
    {
        cin >> S[i].timeIn >> S[i].timeOut >> S[i].amount;
        S[i].index = i;
        
        for(int j = 1; j <= S[i].amount; j++)        //输入每条船的货物信息
        {
            cin >> S[i].c[j].time >> S[i].c[j].destinationTime;
            S[i].c[j].destinationTime = W[S[i].c[j].destinationTime].time;
            S[i].c[j].index = j;
            S[i].c[j].shipIndex = i;
        }
    }
    
    Chromosome curPopulation[populationSize];       //本种群
    Chromosome nxtPopulation[populationSize];       //下代种群
    Chromosome bestIndl;                            //最优适应度个体
    Chromosome emptyOne;                            //空
    
    //初始化
    for(int i = 0; i < chromosomeLen; i++)
    {
        emptyOne.prioritySequence[i] = 0;
    }
    emptyOne.fitAccumulate = 0;
    emptyOne.fitRate = 0;
    emptyOne.fitValue = 0;
    
    bestIndl = emptyOne;
    for(int i = 0; i < populationSize; i++)
    {
        curPopulation[i] = emptyOne;
        nxtPopulation[i] = emptyOne;
    }
    
    init(curPopulation);
    update(curPopulation);
    
    for(int i = 0; i < generationNum; i++)
    {
        select(curPopulation, nxtPopulation, bestIndl);     //选择
        crossover(nxtPopulation);                           //交叉
        mutation(nxtPopulation);                            //变异
        update(nxtPopulation);                              //更新
        
        for(int i = 0; i < populationSize; i++)             //新的子代继续遗传
        {
            curPopulation[i] = nxtPopulation[i];
            nxtPopulation[i] = emptyOne;
        }
    }
    
    cout << bestIndl.time << endl;
    for(int i = 0; i < chromosomeLen; i++)
    {
        cout << bestIndl.prioritySequence[i] <<" ";
    }
    cout<<endl;
    
    end=clock();
    printf("totile time=%f\n",(float)(end-start)*1000/CLOCKS_PER_SEC);
    
    
    return 0;
}

void init(Chromosome (&c)[populationSize])
{
    for(int i = 0; i < populationSize; i++)
    {
        for(int j = 0; j < chromosomeLen; j++)
        {
            c[i].prioritySequence[j] = j + 1;   //给每个染色体初始排序
        }
    }
    
    for(int i = 0; i < populationSize; i++)
    {
        for(int j = 0; j < chromosomeLen; j++)
        {
            //生成随机数种子
            long long seed = chrono::system_clock::now().time_since_epoch().count();
            shuffle(c[i].prioritySequence.begin(), c[i].prioritySequence.end(), default_random_engine((unsigned)seed)); //随机序列顺序
        }
    }
    
}
//适应度（所用时长，越高越好）
double calFit(Chromosome (&cur)[populationSize], int idx)
{
    int totalTime = 0;      //总消耗时间
    
    for(int i = 0; i < 4; i++)
    {
        if(cur[idx].prioritySequence[i] > 4)//非法串，和其后非法位置交换
        {
            for(int j = 4; j < chromosomeLen; j++)
            {
                if(cur[idx].prioritySequence[j] <= 4)
                {
                    int temp = cur[idx].prioritySequence[i];
                    cur[idx].prioritySequence[i] = cur[idx].prioritySequence[j];
                    cur[idx].prioritySequence[j] = temp;
                    break;
                }
            }
        }
    }
    
    for(int i = 0; i < 4; i++)//船入队列
    {
        S[cur[idx].prioritySequence[i]].priority = i;
        qs.push(S[cur[idx].prioritySequence[i]]);
    }
    
    //输入优先级
    int k = 4;
    for(int i = 1; i <= shipN; i++)
    {
        for(int j = 1; j <= S[i].amount; j++)
        {
            S[i].c[j].priority = cur[idx].prioritySequence[k];
            k++;
        }
    }
    ship S2[4 + 1];              //船
    
    memcpy(S2, S, sizeof(S));
    //计算总时长
    while(totalTime >= 0)
    {
        if(isAllEmpty(S) && isAllEmpty(V) && isEmpty(D) && isEmpty(&A) && isAllEmpty(B))//货物都运送完毕&&船离开,结束计时
            break;
        totalTime++;//+1min
        
        //入泊部分
        while(isEmptyIndex(B) && notParkIndex(S) )  //有泊位空了&&有船未入泊，入船
        {
            int curBerth = isEmptyIndex(B);         //第一个空泊位
            int curShip = qs.top().index;           //队列里首艘船
            
            for(int i = 1; i <= qs.top().amount; i++) //每入泊一艘船，起吊队列就刷新
                C.push(qs.top().c[i]);
            
            B[curBerth].shipOrd = curShip;
            B[curBerth].timeIn = S[curShip].timeIn; //计入时间
            S[curShip].isPark = 1;                  //已经停泊了
            qs.pop();                               //已经入泊的船出队列
        }
        
        //起吊部分
        for(int j = 1; j <= deviceN; j++)
        {
            int flag = 0;                                       //是否起吊
            if(D[j].busy == 0)                                  //空闲塔吊
            {
                int curBestB = 0, curBestS = 0;
                for(int i = 1; i <= berthN; i++)                //遍历泊位,起吊
                {
                    if(B[i].shipOrd != 0 && S[B[i].shipOrd].amount != 0 && B[i].timeIn == 0)   //有船停着&&仍有货&&船静止允许起吊
                    {
                        curBestB = i;
                        curBestS = C.top().shipIndex;
                        flag = 1;
                    }
                }
                if(flag == 1)                   //可以起吊
                {
                    D[j].busy = 1;
                    D[j].time = C.top().time;
                    D[j].shipOrd = curBestS;    //来自哪艘船
                    D[j].dstiTime = C.top().destinationTime;
                    C.pop();                    //出队列
                    S[curBestS].amount--;       //剩余货物--
                }
            }
        }
        
        //离港部分
        for(int i = 1; i <= berthN; i++)
        {
            int working = 0;                            //还在工作
            if(S[B[i].shipOrd].amount == 0)                //港里停的船卸空了
            {
                for(int j = 1; j <= deviceN; j++)       //确认塔吊没在吊它的货
                {
                    if(D[j].shipOrd == B[i].shipOrd)    //还在吊
                    {
                        working = 1;
                        break;
                    }
                }
                if(working == 0 && B[i].shipOrd != 0 && B[i].timeOut == 0)  //吊装结束
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
                    V[i].time = qa.top();               //运送时间转移给车
                    qa.pop();                           //出队列
                    A.amount--;
                    V[i].busy = 1;                      //运货中
                }
            }
        }
        
        //计时部分
        for(int i = 1; i <= berthN; i++)        //检查每个泊位的入泊与驶出时间
        {
            
            if(B[i].timeIn != 0)                //减少时间（入）
            {
                B[i].timeIn--;
            }
            if(B[i].timeOut != 0)               //减少时间（出）
            {
                B[i].timeOut--;
                if(B[i].timeOut == 0)           //离港完成
                {
                    B[i].shipOrd = 0;           //空
                }
            }
        }
        for(int i = 1; i <= deviceN; i++)       //检查每个塔吊的时间
        {
            if(D[i].time != 0 && D[i].busy)     //在吊装
            {
                D[i].time--;
                if(D[i].time == 0)              //吊装完成
                {
                    D[i].busy = 0;
                    D[i].shipOrd = 0;
                    qa.push(D[i].dstiTime);     //运送时间入队列
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
    
    memcpy(S, S2, sizeof(S));
    
    return totalTime;
}

void update(Chromosome (&c)[populationSize])
{
    double all = 0;
    for(int i = 0; i < populationSize; i++)
    {
        c[i].time = calFit(c, i);
        c[i].fitValue = (double)1000 / (double)c[i].time;//更新适应度
        all += c[i].fitValue;                         //标准化
    }
    
    //计算适应百分比和累积百分比，用于轮盘赌
    c[0].fitRate = c[0].fitValue / all;
    c[0].fitAccumulate = c[0].fitRate;
    for(int j = 1; j < populationSize; j++)
    {
        c[j].fitRate = c[j].fitValue / all;
        c[j].fitAccumulate = c[j].fitRate + c[j - 1].fitAccumulate;
    }
}

void select(Chromosome (&c)[populationSize], Chromosome (&nextc)[populationSize], Chromosome &bestOne)
{
    int i = 0;
    int j = 0;
    double randRate = 0.0;
    
    bestOne = c[0];
    //生成随机数
    srand((unsigned)time(NULL));
    
    for(i = 0; i < populationSize; i++)
    {
        
        randRate = (double)rand() / RAND_MAX;
        if(randRate < c[0].fitAccumulate)
            nextc[i] = c[0];                    //概率合适，被选中进入下一代
        else
        {
            for(j = 0; j < populationSize; j++)
            {
                if(c[j].fitAccumulate <= randRate && c[j + 1].fitAccumulate > randRate)
                {
                    nextc[i] = c[j + 1];        //概率落在区间内，被选中进入下一代
                    break;
                }
            }
        }
        
        if(c[i].fitValue > bestOne.fitValue)
        {
            bestOne = c[i];
        }
    }
}

//交叉：采用单交叉点法
void crossover(Chromosome (&nextc)[populationSize])
{
    double randRate = 0.0;
    int temp = 0;
    int randN1 = 0;
    int randN2 = 0;
    int posRand = 0;
    
    //随机种子
    srand((unsigned)time(NULL));
    
    for(int i = 0; i < populationSize; i++)
    {
        randRate = (double)rand() / (RAND_MAX);
        //概率小于交叉概率，执行交叉
        if(randRate <= crossoverRate)
        {
            randN1 = (int)rand()%(populationSize);//染色体1
            randN2 = (int)rand()%(populationSize);//染色体2
            while(randN1 == randN2)
                randN2 = (int)rand()%(populationSize);
            
            posRand = (int)rand()%(chromosomeLen - 1);//交叉位置
            
            
            Chromosome temp1, temp2;
            for(int j = 0; j < chromosomeLen; j++)  //清空temp
            {
                temp1.prioritySequence[j] = 0;
                temp2.prioritySequence[j] = 0;
            }
            
            //生成子代1
            for(int j = 0; j < posRand; j++)
            {
                temp1.prioritySequence[j] = nextc[randN1].prioritySequence[j];
            }
            int pos = posRand;                           //队尾
            for(int j = 0; j < chromosomeLen; j++)//扫描randN2
            {
                int find = 0;                           //是否找到重复点
                for(int k = 0; k < chromosomeLen; k++)//扫描temp
                {
                    if(nextc[randN2].prioritySequence[j] == temp1.prioritySequence[k])
                    {
                        find = 1;
                        break;
                    }
                }
                if(find == 0)//没找到重复的，可以放入
                {
                    temp1.prioritySequence[pos] = nextc[randN2].prioritySequence[j];
                    pos++;
                }
            }
            
            //生成子代2
            for(int j = 0; j < posRand; j++)
            {
                temp2.prioritySequence[j] = nextc[randN2].prioritySequence[j];
            }
            pos = posRand;                           //队尾
            for(int j = 0; j < chromosomeLen; j++)//扫描randN2
            {
                int find = 0;                           //是否找到重复点
                for(int k = 0; k < chromosomeLen; k++)//扫描temp
                {
                    if(nextc[randN1].prioritySequence[j] == temp2.prioritySequence[k])
                    {
                        find = 1;
                        break;
                    }
                }
                if(find == 0)//没找到重复的，可以放入
                {
                    temp2.prioritySequence[pos] = nextc[randN1].prioritySequence[j];
                    pos++;
                }
            }
            
            nextc[randN1] = temp1;
            nextc[randN2] = temp2;
            
        }
    }
}


//变异
void mutation(Chromosome (&nextc)[populationSize])
{
    int posRand = 0;
    double randRate = 0.0;
    
    srand((unsigned)time(NULL));//seed
    
    for(int i = 0; i < populationSize; i++)//变异populationSize次,交换一个位点和其后（前）位点
    {
        randRate = (double)rand() / (RAND_MAX);
        
        if(randRate <= mutationRate)
        {
            posRand = (int)rand() % (chromosomeLen);
            
            
            if(posRand != 3 && posRand != chromosomeLen - 1)         //和后面交换
            {
                int temp = nextc[i].prioritySequence[posRand];
                nextc[i].prioritySequence[posRand] = nextc[i].prioritySequence[posRand + 1];
                nextc[i].prioritySequence[posRand + 1] = temp;
            }
            else                                    //和前面交换
            {
                int temp = nextc[i].prioritySequence[posRand];
                nextc[i].prioritySequence[posRand] = nextc[i].prioritySequence[posRand - 1];
                nextc[i].prioritySequence[posRand - 1] = temp;
            }
        }
    }
}
