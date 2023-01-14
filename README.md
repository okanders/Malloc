# 7-malloc
You should provide a README file which documents the following:
a description of your strategy for maintaining compaction (i.e. how are you preventing your heap from turning into a bunch of tiny free blocks?)
your mm_realloc() implementation strategy
your strategy to achieve high throughput (see next point on Performance)
unresolved bugs with your program
any other optimizations




    For optimization, I would like to highlight two moves in my malloc, and then my overall strategy for realloc. Throughout the program, I make sure to coalesce an free blocks that seem to be created first off, which helps with preventing any tiny free blocks. In malloc, I make sure that whenever I am extending my heap, that I check to see if there is a free block before my epilogue that can be extended. I also provide a chunk malloc concept that if the alignedsize<128, I choose to extend by the chunk—this prevents small free blocks. I also choose to only allow free blocks to be created, when split, that are miniblocksize or greater.

    In realloc, I really try to reallocate the given block, checking the next block and the previous block to see if their free. I check the next block and was able to implement a successful reallocation, but in terms of optimization for the previous block, I was unable to figure out how to reallocate in the best format. I ultimately decided to just go through the free list and malloc the new size.# Malloc
# Malloc
