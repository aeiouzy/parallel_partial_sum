/*
ȷ��ĳ����Χ���ֺ͵�һ�ֵķ�ʽ�������ڶ������м��㲿�ֺͣ�Ȼ�󽫵�һ��������Ԫ
�ص�ֵ������һ���е�����Ԫ�ؽ�����ӣ��������ơ�����и�����[1��2��3��4��5��6��7��8��9]��
Ȼ�����Ϊ���飬��ô�ڵ�һ�μ������ܵõ�[{1��3��6}��{4��9��15}��{7��15��24}]��
Ȼ��6(��һ������һ��Ԫ��)�ӵ��ڶ������У���ô�͵õ�[{1��3��6}��{10��15��21}��
{7��15��24}]��Ȼ���ٽ��ڶ�������һ��Ԫ��21�ӵ���������ȥ���͵õ�[{1��3��6}��
{10��15��21}��{28��36��55}]��
*/
#include <future>
#include <numeric>

template<typename Iterator>
void parallel_partial_sum(Iterator first, Iterator last)
{
	typedef typename Iterator::value_type value_type;
	struct process_chunk//1
	{
		void operator()(Iterator begin, Iterator last,
			std::future<value_type>* previous_end_value,
			std::promise<value_type>* end_value)
		{
			try
			{
				Iterator end = last;
				++end;
				std::partial_sum(begin, end, begin);//2
				if (previous_end_value)//3
				{
					value_type& addend = previous_end_value->get();//4
					*last += addend;//5
					if (end_value)
						end_value->set_value(*last);//6
					std::for_each(begin, last, [addend](value_type& item)
					{item += addend; });
				}
				else if (end_value)
					end_value->set_value(*last);//8
			}
			catch (...)//9
			{
				if (end_value)
					end_value->set_exception(std::current_exception());//10
				else
					throw;//11
			}
		}
	};

	unsigned long const length = std::distance(first, last);
	if (!length)
		return last;

	unsigned long const min_per_thread = 25;//12
	unsigned long const max_threads =
		(length + min_per_thread - 1) / min_per_thread;

	unsigned long const hardware_threads =
		std::thread::hardware_concurrency();

	unsigned long const num_threads =
		std::min(hardware != 0 ? hardware_threads : 2, max_threads);

	unsigned long const block_size = length / num_threads;

	typedef typename Iterator::value_type value_type;

	std::vector<std::thread> threads(num_threads - 1);//13
	std::vector<std::promise<value_type>> end_values(num_threads - 1);//14
	std::vector<std::future<value_type>> previous_end_values;//15
	previous_end_values.reserve(num_threads - 1);//16
	join_threads joiner(threads);

	Iterator block_start = first;
	for (unsigned long i = 0; i < (num_threads - 1); ++i)
	{
		Iterator block_last = block_start;
		std::advance(block_last, block_size - 1);//17
		threads[i] = std::thread(process_chunk(),//18
			block_start, block_last,
			(i != 0) ? &previous_end_values[i - 1] : 0, &end_values[i]);
		block_start = block_last;
		++block_start;//19
		previous_end_values.push_back(end_values[i].get_future());//20
	}
	Iterator final_elememt = block_start;
	std::advance(final_elememt, std::distance(block_start, last - 1));//21
	process_chunk()(block_start, final_elememt,//22
		(num_threads > 1) ? &previous_end_values.back() : 0, 0);
}