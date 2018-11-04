#ifndef GETHAIKUPLUGIN_H
#define GETHAIKUPLUGIN_H

#include <random>
#include <iterator>
#include <vector>

#include <Message.h>

template<typename Iter, typename RandomGenerator>
Iter select_randomly(Iter start, Iter end, RandomGenerator& g)
{
	std::uniform_int_distribution<> dis(0, std::distance(start, end) - 1);
	std::advance(start, dis(g));
	return start;
}

template<typename Iter>
Iter select_randomly(Iter start,Iter end)
{
	static std::random_device rd;
	static std::mt19937 gen(rd());
	return select_randomly(start, end, gen);
}

class PoemManager
{
public:
											PoemManager(std::vector<std::string> poemlist);
	virtual									~PoemManager();
				void						MessageReceived(BMessage* message);
private:
				const char*					GetRandom();
				const char*					AtIndex(uint32 idx);
				const uint32				Count();
				const uint32				AddPoem(const char* poem);
				
				std::vector<std::string> 	poems;
};

#endif
