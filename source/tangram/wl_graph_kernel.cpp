#include "global_graph_builder.h"



void test()
{
	// 遍历所有图的所有顶点，给feature标号
	
	// 设矩阵顶点数为n，边数为m
	// 首先找出所有顶点最大的出度 odmx 和 最大的 入度 idmx

	// 建立两个矩阵
	// 矩阵1存储所有顶点的出边，n * odmx
	// 矩阵2存储所有顶点的入边，n * idmx

	// 一个数组存储所有顶点的hash value 大小 n

	// 遍历出边矩阵
	// 访问label，hash所有出边节点，

	// 对找出的所有feature进行排序，挑选指定数量的feature，原则为 feature 数量之和大于整个图的权重的120%


	// 1 / 2 / 3 / 4 /5 阶的feature分开存储，每阶feature不同
}