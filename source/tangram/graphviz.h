#pragma once
#include <stdint.h>
#include <string>
#include <vector>

struct SNodeDesc
{
	std::string lable;
};

struct SNode
{
	std::string lable;
};

struct SEdge
{
	uint32_t from;
	uint32_t to;
};

class CGraphVis
{
public:
	
	uint32_t addNode(const SNodeDesc& node_desc);
	inline void addEdge(uint32_t from, uint32_t to)
	{
		edges.push_back(SEdge{ from ,to });
	}
	void generate();
private:
	std::vector<SNode> nodes;
	std::vector<SEdge> edges;
};