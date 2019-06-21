#pragma once
#include <vector>

using namespace std;

#define MIN(a,b) (((a)<(b))?(a):(b))

typedef unsigned char uchar;

template <class TWeight>
class GCGraph
{
public:
	GCGraph();
	GCGraph(unsigned int vtxCount, unsigned int edgeCount);
	~GCGraph();
	void create(unsigned int vtxCount, unsigned int edgeCount); //给图的结点容器和边容器分配内存
	int addVtx(); //添加空结点
	void addEdges(int i, int j, TWeight w, TWeight revw); //添加点之间的边n-link
	void addTermWeights(int i, TWeight sourceW, TWeight sinkW); //添加结点到顶点的边t-link
	TWeight maxFlow(); //最大流函数
	bool inSourceSegment(int i); //图对象调用最大流函数后，判断结点属不属于属于源点类（前景）
private:
	class Vtx  //结点类
	{
	public:
		Vtx * next; //在maxflow算法中用于构建先进-先出队列
		int parent;
		int first; //首个相邻边
		int ts; //时间戳
		int dist; //到树根的距离
		TWeight weight;
		uchar t; //图中结点的标签，取值0或1，0为源节点（前景点），1为汇节点（背景点）
	};

	class Edge //边类
	{
	public:
		int dst; //边指向的结点
		int next; //该边的顶点的下一条边
		TWeight weight; //边的权重
	};

	std::vector<Vtx> vtcs; //存放所有的结点
	std::vector<Edge> edges; //存放所有的边
	TWeight flow; //图的流量
};

template <class TWeight>
GCGraph<TWeight>::GCGraph()
{
	flow = 0;
}
template <class TWeight>
GCGraph<TWeight>::GCGraph(unsigned int vtxCount, unsigned int edgeCount)
{
	create(vtxCount, edgeCount);
}

template <class TWeight>
GCGraph<TWeight>::~GCGraph()
{
}
template <class TWeight>
void GCGraph<TWeight>::create(unsigned int vtxCount, unsigned int edgeCount) //构造函数的实际内容，根据节点数和边数
{
	vtcs.reserve(vtxCount);
	edges.reserve(edgeCount + 2);
	flow = 0;
}

/*
函数功能：添加一个空结点，所有成员初始化为空
参数说明：无
返回值：当前结点在集合中的编号
*/
template <class TWeight>
int GCGraph<TWeight>::addVtx()
{
	Vtx v;
	memset(&v, 0, sizeof(Vtx)); //将结点申请到的内存空间全部清0(第二个参数0)  目的：由于结点中存在成员变量为指针，指针设置为null保证安全
	vtcs.push_back(v);
	return (int)vtcs.size() - 1; //返回值：当前结点在集合中的编号
}

/*
函数功能：添加一条结点i和结点j之间的边n-link(普通结点之间的边)
参数说明：
int---i: 弧头结点编号
int---j: 弧尾结点编号
Tweight---w: 正向弧权值
Tweight---reww: 逆向弧权值
返回值：无
*/
template <class TWeight>
void GCGraph<TWeight>::addEdges(int i, int j, TWeight w, TWeight revw)
{
	assert(i >= 0 && i < (int)vtcs.size());
	assert(j >= 0 && j < (int)vtcs.size());
	assert(w >= 0 && revw >= 0);
	assert(i != j);

	Edge fromI, toI; // 正向弧：fromI, 反向弧 toI

	fromI.dst = j; // 正向弧指向结点j
	fromI.next = vtcs[i].first; //每个结点所发出的全部n-link弧(4个方向)都会被连接为一个链表，采用头插法插入所有的弧
	fromI.weight = w; // 正向弧的权值w 
	vtcs[i].first = (int)edges.size(); //修改结点i的第一个弧为当前正向弧
	edges.push_back(fromI); //正向弧加入弧集合

	toI.dst = i;
	toI.next = vtcs[j].first;
	toI.weight = revw;
	vtcs[j].first = (int)edges.size();
	edges.push_back(toI);
}

/*
函数功能：为结点i的添加一条t-link弧(到终端结点的弧),添加节点的时候,同时调用此函数
参数说明：
int---i: 结点编号
Tweight---sourceW: 正向弧权值
Tweight---sinkW: 逆向弧权值
返回值：无
*/
template <class TWeight>
void GCGraph<TWeight>::addTermWeights(int i, TWeight sourceW, TWeight sinkW)
{
	assert(i >= 0 && i < (int)vtcs.size());

	TWeight dw = vtcs[i].weight;
	if (dw > 0)
		sourceW += dw;
	else
		sinkW -= dw;
	flow += (sourceW < sinkW) ? sourceW : sinkW;
	vtcs[i].weight = sourceW - sinkW;
}


/*
函数功能：最大流函数，将图的所有结点分割为源点类（前景）还是汇点类（背景）
参数：无
返回值：图的成员变量--flow
*/
template <class TWeight>
TWeight GCGraph<TWeight>::maxFlow()
{
	const int TERMINAL = -1, ORPHAN = -2;
	Vtx stub, *nilNode = &stub, *first = nilNode, *last = nilNode;//先进先出队列，保存当前活动结点，stub为哨兵结点
	int curr_ts = 0; //当前时间戳
	stub.next = nilNode; //初始化活动结点队列，首结点指向自己
	Vtx *vtxPtr = &vtcs[0]; //结点指针
	Edge *edgePtr = &edges[0]; //弧指针

	vector<Vtx*> orphans; //孤立点集合

						  // 遍历所有的结点，初始化活动结点(active node)队列  
	for (int i = 0; i < (int)vtcs.size(); i++)
	{
		Vtx* v = vtxPtr + i;
		v->ts = 0;
		if (v->weight != 0) //当前结点t-vaule(即流量)不为0
		{
			last = last->next = v; //入队，插入到队尾
			v->dist = 1; //路径长度记1
			v->parent = TERMINAL; //标注其双亲为终端结点
			v->t = v->weight < 0;
		}
		else
			v->parent = 0; //孤结点
	}
	first = first->next; //首结点作为哨兵使用，本身无实际意义，移动到下一节点，即第一个有效结点
	last->next = nilNode; //哨兵放置到队尾了。。。检测到哨兵说明一层查找结束
	nilNode->next = 0;


	//很长的循环，每次都按照以下三个步骤运行：  
	//搜索路径->拆分为森林->树的重构
	for (;;)
	{

		Vtx* v, *u; // v表示当前元素，u为其相邻元素
		int e0 = -1, ei = 0, ej = 0;
		TWeight minWeight, weight; // 路径最小割(流量)， weight当前流量
		uchar vt; // 流向标识符，正向为0，反向为1

				  //----------------------------              第一阶段: S 和 T 树的生长，找到一条s->t的路径             -------------------------//  
		while (first != nilNode)
		{
			v = first; // 取第一个元素存入v，作为当前结点
			if (v->parent) // v非孤儿点
			{
				vt = v->t; // 纪录v的流向

						   // 广度优先搜索，以此搜索当前结点所有相邻结点， 方法为：遍历所有相邻边，调出边的终点就是相邻结点  
				for (ei = v->first; ei != 0; ei = edgePtr[ei].next)
				{
					// 每对结点都拥有两个反向的边，ei^vt表明检测的边是与v结点同向的
					if (edgePtr[ei^vt].weight == 0)
						continue;
					u = vtxPtr + edgePtr[ei].dst; // 取出邻接点u
					if (!u->parent) // 无父节点，即为孤儿点，v接受u作为其子节点
					{
						u->t = vt; // 设置结点u与v的流向相同
						u->parent = ei ^ 1; // ei的末尾取反。。。
						u->ts = v->ts; // 更新时间戳，由于u的路径长度通过v计算得到，因此有效性相同  
						u->dist = v->dist + 1; // u深度等于v加1
						if (!u->next) // u不在队列中，入队，插入位置为队尾
						{
							u->next = nilNode; // 修改下一元素指针指向哨兵
							last = last->next = u; // 插入队尾
						}
						continue;
					}

					if (u->t != vt) // u和v的流向不同，u可以到达另一终点，则找到一条路径
					{
						e0 = ei ^ vt;
						break;
					}

					// u已经存在父节点，但是如果u的路径长度大于v+1，且u的时间戳较早，说明u走弯路了，修改u的路径，使其成为v的子结点    
					if (u->dist > v->dist + 1 && u->ts <= v->ts)
					{
						// reassign the parent
						u->parent = ei ^ 1; // 从新设置u的父节点为v(编号ei)，记录为当前的弧
						u->ts = v->ts; // 更新u的时间戳与v相同
						u->dist = v->dist + 1; // u为v的子结点，路径长度加1
					}
				}
				if (e0 > 0)
					break;
			}
			// exclude the vertex from the active list
			first = first->next;
			v->next = 0;
		}

		if (e0 <= 0)
			break;

		//-----------------------------------                第二阶段: 流量统计与树的拆分           ---------------------------------------//  
		//第一节： 查找路径中的最小权值 
		minWeight = edgePtr[e0].weight;
		assert(minWeight > 0);
		// 遍历整条路径分两个方向进行，从当前结点开始，向前回溯s树，向后回溯t树  
		// 2次遍历， k=1: 回溯s树， k=0: 回溯t树
		for (int k = 1; k >= 0; k--)
		{
			//回溯的方法为：取当前结点的父节点，判断是否为终端结点  
			for (v = vtxPtr + edgePtr[e0^k].dst;; v = vtxPtr + edgePtr[ei].dst)
			{
				if ((ei = v->parent) < 0)
					break;
				weight = edgePtr[ei^k].weight;
				minWeight = MIN(minWeight, weight);
				assert(minWeight > 0);
			}
			weight = fabs(v->weight);
			minWeight = MIN(minWeight, weight);
			assert(minWeight > 0);
		}

		/*第二节：修改当前路径中的所有的weight权值
		任何时候s和t树的结点都只有一条边使其连接到树中，当这条弧权值减少为0则此结点从树中断开，
		若其无子结点，则成为孤立点，若其拥有子结点，则独立为森林，但是ei的子结点还不知道他们被孤立了！
		*/
		edgePtr[e0].weight -= minWeight; //正向路径权值减少
		edgePtr[e0 ^ 1].weight += minWeight; //反向路径权值增加
		flow += minWeight; //修改当前流量

						   // k = 1: source tree, k = 0: destination tree
		for (int k = 1; k >= 0; k--)
		{
			for (v = vtxPtr + edgePtr[e0^k].dst;; v = vtxPtr + edgePtr[ei].dst)
			{
				if ((ei = v->parent) < 0)
					break;
				edgePtr[ei ^ (k ^ 1)].weight += minWeight;
				if ((edgePtr[ei^k].weight -= minWeight) == 0)
				{
					orphans.push_back(v);
					v->parent = ORPHAN;
				}
			}

			v->weight = v->weight + minWeight * (1 - k * 2);
			if (v->weight == 0)
			{
				orphans.push_back(v);
				v->parent = ORPHAN;
			}
		}

		//----------------------------                第三阶段: 树的重构 寻找新的父节点，恢复搜索树               -----------------------------//
		curr_ts++;
		while (!orphans.empty())
		{
			Vtx* v = orphans.back(); //取一个孤儿
			orphans.pop_back(); //删除栈顶元素，两步操作等价于出栈

			int d, minDist = INT_MAX;
			e0 = 0;
			vt = v->t;

			//  遍历当前结点的相邻点，ei为当前弧的编号
			for (ei = v->first; ei != 0; ei = edgePtr[ei].next)
			{
				if (edgePtr[ei ^ (vt ^ 1)].weight == 0)
					continue;
				u = vtxPtr + edgePtr[ei].dst;
				if (u->t != vt || u->parent == 0)
					continue;

				// 计算当前点路径长度
				for (d = 0;;)
				{
					if (u->ts == curr_ts)
					{
						d += u->dist;
						break;
					}
					ej = u->parent;
					d++;
					if (ej < 0)
					{
						if (ej == ORPHAN)
							d = INT_MAX - 1;
						else
						{
							u->ts = curr_ts;
							u->dist = 1;
						}
						break;
					}
					u = vtxPtr + edgePtr[ej].dst;
				}

				// update the distance
				if (++d < INT_MAX)
				{
					if (d < minDist)
					{
						minDist = d;
						e0 = ei;
					}
					for (u = vtxPtr + edgePtr[ei].dst; u->ts != curr_ts; u = vtxPtr + edgePtr[u->parent].dst)
					{
						u->ts = curr_ts;
						u->dist = --d;
					}
				}
			}

			if ((v->parent = e0) > 0)
			{
				v->ts = curr_ts;
				v->dist = minDist;
				continue;
			}

			/* no parent is found */
			v->ts = 0;
			for (ei = v->first; ei != 0; ei = edgePtr[ei].next)
			{
				u = vtxPtr + edgePtr[ei].dst;
				ej = u->parent;
				if (u->t != vt || !ej)
					continue;
				if (edgePtr[ei ^ (vt ^ 1)].weight && !u->next)
				{
					u->next = nilNode;
					last = last->next = u;
				}
				if (ej > 0 && vtxPtr + edgePtr[ej].dst == v)
				{
					orphans.push_back(u);
					u->parent = ORPHAN;
				}
			}
		}
		//第三阶段结束

	}
	return flow; //返回最大流量
}

/*
函数功能：判断结点是不是源点类（前景）
参数：结点在容器中位置
返回值：1或0，1：结点为前景，0：结点为背景
*/
template <class TWeight>
bool GCGraph<TWeight>::inSourceSegment(int i)
{
	assert(i >= 0 && i < (int)vtcs.size());
	return vtcs[i].t == 0;
};