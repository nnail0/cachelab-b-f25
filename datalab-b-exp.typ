#set text(font: "New Computer Modern")

#align(center)[
  #text(size: 20pt)[CS 341L Cachelab Part B Report \ ]
  #text(size: 16pt)[ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ Roxanne Lutz, Nathan Nail | Fall 2025 \ 8 December 2025 \ University of New Mexico]
]


== Summary


== Introduction and Motivation


== Methods

This project was primarily split into two natural halves between the two partners: 
+ One person's focus was upon, entirely from scratch, approaching the optimization by hand.
+ The other person's focus was on the generation of solutions with generative AI. 

The following parts are the methodolgies used in generating potential solutions.

=== Human
The human generated code was developed in the following order:
+ _32x32_ optimization
+ _64x64_ optimization
+ _61x67_ optimization

The most useful approach to each of these problems was developed by the approach to the first. Picture/color representations of the set overlays on the matrics were generated to a degree in which the problematic miss rate areas were visibly simple to identify. Then, as will be discussed in the data collection portion of this report, solutions to mitigate these miss rates were developed based upon successful changes.

=== GenAI
The genAI code was developed with ChatGPT 5 mini (with reasoning), which is the version of ChatGPT offered on a trial basis to those who have an OpenAI account but do not pay for any other tier. The model opted to approach the code in a rather monolothic manner and generated code for all three cases to put into one function. 

When provided with code from the model, I replaced whatever code was already in the `transpose-submit` function and performed a direct replacement of all of the code, even if 1 or 2 out of the three ended up correct during the previous iteration. This was likely a suboptimal approach, but it demonstrates the deficiencies of ChatGPT's "memory". It likely would have been beneficial to split up the code generation into three different chunks to allow the model to analyze each case separately. The results of this method will be discussed later in the report. 

== Data Collection
The following section goes into depth on the processes of generating solutions, both human and genAI alike. 

=== Human


==== Code Generation
The following discussion of code generation will be presented from worst attempt to best attempt (or, the final submission that accomplishes a miss cound under the minimum bound for each matrix). It should be noted that the attempts were not necessarily in a purely successful order, but will be presented as such for clarity.

The subsections as follow are the order, as mentioned, the human code was generated on overall. They will include, in context, a discussion of attempts made, what was learned from the failure of the attempt, leading to how the final optimized function was created.

Then, there will be a brief discussion of the process itself, along with any difficulties in generating ideas, collecting data, or anything else worth noting.

===== _32x32_

As with all of the attempts made, the initial _32x32_ baseline miss rate was found through the simple trans function given. At worst, the number of misses was 1184. A far-cry from where the miss bound needed to be at < 300, there was some exploring to be done on how the sets of the cache laid.

There are some pieces of key mathematics needed to understand how the sets of this direct-access cache lay and how we can best use it. With the numbers given, as discussed, we have 32 sets, and each set can hold 32 bytes, which will end up holding 8 ints in a set.

For the _32x32_, we can see a couple key patterns. First, the sets exactly fit within the dimensions, the first row being sets 0 - 3, next row being sets 4 - 7, and on. Further, the sets repeat their row overlay every 8 rows. 
 
Therefore, when we are transposing, if we store a value in B and access in A, it is in the best interest in that cache to stay within those sets as much as possible. So, the first attempt past a baseline transposition attempts to make use of this fact and conduct a straightforward transpose procedure but within 8x8 blocks at a time. By staying within these 8x8 blocks, we can make use of the 8 int values loaded into the cache lines as much as possible before moving on to the next 8x8 blocks. 

This attempt drops the miss rate significantly, only having 344 misses. From there, we need to analyze where the worst of the misses occur. We can see, by assuming and then verifying empirically by the cache simulation addresses, that A and B can be treated as contiguous in memory. The significance of this is that the sets of the cache overlap in the current access pattern specifically at the diagonal blocks (rows/columns 0 - 7, 8 - 15, …, 24 - 31). When we transpose these specific 8x8 blocks, accessing A and storing in B, we are thrashing the sets heavily due to this overlap.

So, one way to address this is to load things preemptively, and allow one matrix full control, so to speak, over a set until it is ready to relinquish control to the other. To accomplish this, we will load a diagonal value of A into a temporary variable (for example, temp = A[0][0]). This allows A to claim control over the set overlaying this small block row (A[0] allows A control over set 0, following our example). Then, we allow B to have control over all the other sets, setting all values of B except for the diagonal (avoid B[0][0], set B[1..7][0] = A[0][1..7]). This allows A to hit on all accesses in this iteration past the initial access. Then, when we are done allowing A to have control of a set, we hand over control to B, storing the temporary value grabbed at the beginning into the appropriate diagonal slot (B[0][0] = temp). With this attempt at careful control hand-off between sets on the diagonal blocks, we manage to plummet the miss rate below our boundary: 288 misses.

===== _64x64_

As with the 32x32, we will start with a baseline of running the trans function to identify the worst-case miss rate, which comes in far higher than the prior square at 4724 misses.

As an initial measure, since the 64x64 is also equally divisible into 8x8 blocks, we will attempt the same blocking mechanic as the 32x32, negating the diagonal handling, just to see the behavior untouched. This unfortunately has no tangible effect, having the exact same amount of misses as the baseline: 4724. 

However, with blocking being a remarkably successful method in mitigating the miss rate, we can try changing the blocksize to see if there is any change. After some exploring, a blocksize of 4x4 instead ended up having the best decrease in misses, dropping to 1892.

From here, some closer analysis was required to get that number lower (along with a fair amount of failures). There is a definitive difference in how the 64x64 sets overlay A and B matrices compared to the 32x32. For the 32x32, in each grouping of 8 columns (or 8 ints that will be loaded into a set line), every 8 rows, a group of sets repeat. The 64x64 has the same pattern, but with far more difficult consequences: in the same 8 column groupings, the set groups repeat instead every 4 rows. This is the reason that the 8x8 blocking alone does nothing of use; he filling of B column-wise thrashes itself within the 8x8 block. Nevertheless, that is exactly why the 4x4 block improves: B thrashing decreases considerably by at least hitting instead of missing on those smaller 4 int accesses per row before thrashing itself in the next iteration/block.

We still run into an unfortunate issue: in the general case (we will cover diagonal blocks in a moment), we are not making the best use of the ints loaded into the cache line. When we 4x4 block, we never use the second half of the set line when filling B column-wise. So, there must be an access pattern that minimizes misses between A and B both with filling blocks. Through many drawings, a mixture of the first two attempts was developed. First, we will overall still deal with 8x8 blocks since the cache lines still load in 8 ints at a time and we want to use that fact to our advantage. However, we will change how we fill the 8x8 blocks by using smaller 4x4’s within the larger block. We will focus on the access pattern of A: why not access the smaller 4x4 sections of the 8x8 block as a sideways-U-shape. Meaning, we access the block in A as first to late: upper-left 4x4, upper-right 4x4, bottom-right 4x4, and bottom-left 4x4. This has 2 benefits: (1) we can use all the 8 ints loaded into the set-lines of the top 4 rows on the A-block before allowing A to thrash itself on the bottom 4 rows, using then all of those bottom 8 ints loaded up; (2) we can lessen the self-thrashing of storing in B-blocks since we are filling the 8x8 as an upside-down-U-shape, which admittedly will self-thrash twice, but overall will decrease evictions in the general case. Changing over all blocking to fill using this access pattern decreases the misses further to 1644.

Now, on to the massive problem of the diagonal blocks. The thrashing that occurs at these diagonals is far worse than that of the 32x32, as evidenced by the 8x8 first attempt blocking having no benefit to speak of. Not only is there the typical A and B thrashing each other behavior, but now A and B are additionally thrashing themselves. Not even the U access pattern is enough to fix this problem. We need a unique solution to decrease the miss rate in these blocks specifically to even approach our goal of < 1300 misses.

So, the next attempt utilizes a convoluted loophole. We cannot modify the contents of A, but we can modify B in any way we wish. So, there is no reason we cannot use B as a conversion to get the diagonals to force them into being a general case block fill. The idea is that we can handle the diagonals first as a special case. First, we place the contents of the A-diagonal-block into the 8x8 block next to the B-diagonal block (for the first 7 diagonal blocks, place to the next right 8x8, for the last, place in the next left 8x8). This effectively copies the problematic overlapping-set values into sets that will be guaranteed to not overlap with the B-diagonal-block sets. Next, use the U-shape access pattern to fill the B-diagonal blocks as we did in the prior attempt, but instead fill from the temporary storage block next door. This idea is successful, dropping our tally to 1412 misses. So, this idea was in the correct direction, but needed improvement.

Instead of using the blocks next to each diagonal, we will instead use one consistent storage unit in B for the left half of diagonals, and then a different consistent unit for the right half of diagonals. The choice is somewhat arbitrary, but the following choice has some specific benefits: use rows 0 - 4, columns 48 - 63 for the left half, and use rows 60 - 63, columns 0 - 15 for the right half. The benefit to choosing these spots and filling them with top and bottom rows from A-diagonal-blocks is that, past the initial miss on accessing these 8 sets in B, we will hit every time after as we fill in each iteration from A. These spots in B, further, will not self-thrash when filling B due to them being to the left/right of each other and not above/below. Even better, we can access A row-wise when filling these storage areas, avoiding all except 4 initial misses and 4 self-thrashes. The same holds for B: we can now fill the B-diagonal-blocks row-wise with the needed corresponding values from the B storage units, minimizing to 4 initial misses and 4 self-thrashes. Despite being arbitrarily chosen, we can depend upon these spots for the left/right halves of the matrix to procedurally conduct these diagonal transpositions in a convoluted, but cache-optimized fashion. Then, we use the general case U-shaped access pattern for all other 8x8 blocks in the matrix as we did before. Although the implementation of this is far more difficult than the 5 lines of trans, we finally get below the bound using this technique, arriving at 1268 misses.

===== _61x67_

Yet again, we will consider the baseline miss rate by running the transposition functionality with the given typical trans function. This yields a base miss rate that is about as high as the 64x64 baseline: 4424.

The uniqueness of this size lies in its rectangular shape, no longer being the nicely consistent set overlay that the 32x32 and 64x64 exhibit. By drawing out some of the sets briefly assuming a start at address 0x0 and A (67x61) and B (61x67) being contiguous, we can see that the sets repeat in patterns of 4 as did the 64x64. However, due to the rectangular shape, we do not see the exact 8x8 block issues as the 64x64, the sets repeating in a diagonal pattern instead of straight vertical. This, as intimidating as it initially appears, ends up being a benefit over the squares; the sets do not overlap so horribly at the 8x8 diagonal blocks. 

So, the first attempt made was to simply try blocking tactics that worked well with the other 32x32 and 64x64 sizes. First, we will try to block 8x8 (as the size allows within the rectangle), and fill in the same sideways U-shape used in the 64x64 since the same 4-set repetition pattern occurs here as that size, albeit at an angle. We will leave diagonals alone since they do not align as consistently. That simple access pattern already works very well, yielding 2426 misses. 

Since the usage of normal tactics appears to work well, it seemed worthwhile to ensure that the straightforward approach of an 8x8 blocking (without the smaller 4x4 access patterns) does not provide benefit. Upon running, attempt 2 with a simple 8x8 block actually does end up providing an even lower miss count at 2119. 

With such a close number to the goal boundary of < 2000, this was likely the approach to toy with further. By simply flipping the outer 2 loops, which effectively move the B storage blocks left-right row-wise and A access blocks up-down column-wise, we succeed in getting the misses below the bound, coming in at a final 1914 misses. Surprisingly, the square matrices ended up being the most complicated to analyze, whereas the rectangular size ended up being simpler to optimize toward the goal, especially since it did not have the same horrendous diagonal overlap thrashing. However, it still has a far higher miss rate than the other matrices, making it still far less efficient in terms of cache usage.

==== Discussion
_Notes for roxanne_
+ maybe talk about how long it took to get these ideas?
+ discuss the time taken to consider the set overlays-this was the key way to generate a successful solution; include pngs aside to report? ask soraya
+ unsureness of where to start--initial shooting in dark?
+ discuss benefit of deep understanding gotten from the process of failure despite frustration 


=== GenAI

For the genAI section of the code, ChatGPT 5 mini (with intelligence) was handed the entire PDF, in addition to some instructions I provided to it: 

#block(
  fill: luma(230),
  inset: 8pt,
  radius: 4pt,
  "Hello, I am working on the CS:APP cache lab and was wondering how familiar you are with it. Given the assignment PDF, I was hoping that you could review it, confirm to me that you understand the goals of the assignment, and generate output that meets the criteria to the best of your ability.
"
)

From this, ChatGPT reiterated to me what it understood about the assignment and opted to handle all matrix sizes at the same time. To handle this, it uses if statements and calls to `return` in the case that the 32x32 or 64x64 case is being processed. The 61x67 case is processed until the end of the function call. 

Below, I discuss:
- The results of each evolution of the code. 
- The strategy that the model employed at each setup
- What ended up being the winning strategy for each case, despite the three cases not able to exist in harmony with each other
- Future experiments to conduct with the code to see if improvements are made (i.e. what if, instead of one monolothic function call, there were three smaller function calls for each case?)

==== _32x32_

The 32x32 case, despite being the "simplest",was a case that was difficult for the model to score well on immediately. It took about four attempts to generate code that hit under 300 misses. 
- First run: ran correctly, but was unable to reach the desired miss threshold at 344. 
- Second run: all three cases including 32x32 were 0. This does not seem to be well-defined behavior and indicates to me that the model generated code that broke the tester. 
- Third run: After some troubleshooting to deal with the previous run, this was the first iteration of the code to generate output that yielded 288 runs. This unfortunately came at the cost of breaking one of the other cases in code. 

In addition to succeeding with the 32x32 case, the secondary goal was to remedy issues that were occurring with the final test case of 61x67. Despite instruction to maintain the previously working case, some issues still managed to arise. This leads me to believe that there were some side effects as the result of the "properly working" 64x64 code that went under the miss rate.

The winning strategy for this case ended up being pretty simple, employing 8x8 blocking and solving each block individually with the naive method. With less data to handle at each step, less cache misses were recorded.

==== _64x64_
This code followed a bit of a different story. The development that this code took leads me to believe that there may be some side effects with some of the variables being used, since I am not fully sure what else could be happening. (the model maintained the same structure and order in the code as it did previously). 
- First run: invalid result. This code iteration produced suboptimal yet valid 32x32 code and right-on-the-spot 61x67 code, which was strange. Taking a quick glance at the code, I was never fully able to determine the cause of this issue. 
- Second run: Like with other versions, the code that was generated produced 0 misses, which is effectively impossible and indicates that some sort of issue occurred with this iteration. I prompted ChatGPT to revisit this code and ensure that the setup was being conducted properly. 
- Third run: Like with the 32x32 case, the 64x64 code hit well under the miss target of 1300 and netted 1180 misses. This came at the cost of breaking the 61x67 test case. 

Runs beyond Run 2 were to try and get a round of code that would integrate the successes of these two cases with the first-run success of 61x67; this proved unsuccessful. 

The model's winning strategy proved to be somewhat successful, but after looking at what it took to hit the target miss rate in the human-generated code, this makes sense. From what I was able to understand of the code, it followed three major phases: 
- Handled the first four rows of each 8x8 block in a standard fashion, starting from the top left. 
- Switched to a different strategy that involved breaking it down into further 4x4 blocks and transposing them in a specific order. This seemed to handle two or three of the four 4x4 blocks that the the code partitioned off. This was the only section of code out of the three cases that used the eight extra variables alotted to it. 
- Finished off the final 4x4 block with a more standard transpose techniques. 


==== _61x67_
This test case ended up being one of the simplest, with the best results happening with very few lines of code and with the fewest number of attempts. 

First run - instantly met the miss rate criteria. This code managed to just squeeze under the miss rate of 2,000 at 1,993 misses with about 2 dozen lines of code. 
Second run - like with all of the others, this was invalid due to issues with the code. 
Third run - This was the first run to nail the other two cases, but this (somehow) came at breaking the 61x67 case despite explicitly prompting the model to keep the original working code. I have yet to see whether the reason that the code broke was due to actual changes in the code or due to side effects from previous runs. 

The winning strategy for the 61x67 case was fairly simple and adopted most of the code from the standard 8x8 blocking strategy. The main change involved extra checks on the outer loops (given the irregular size) to handle not writing too much or too little on either of the dimensions. The inner loops employed the naive implementation. 

=== Second Attempt: Three Separate functions
I had a feeling that there were some side effects occurring under the hood as a result of having all three cases in the same function. As a result, I prompted the model to make another attempt. This time, I gave it explicit directions to split the code into three separate cases. This ended up being the solution. 

Upon closer inspection, it appears that ChatGPT added in some corrections to the 32x32 code, which makes me wonder more about where exactly the problem was happening before. Regardless, it appears that the model was able to reflect on its own mistakes from a previous conversation and find what went wrong. This could point to the previously mentioned "short-term memory" issue that I was mentioning earlier, or it could be something different. 

=== Code Quality
Some important aspects in measuring code quality involve legibility, conciseness, and correctness. In all, it looks like the model was able to produce code that I could mostly follow. 
For the 64x64 case, the code was not terribly concise, which made it difficult to parse. However, it was able to generate a satisfactory result. The other cases could be distilled down to simple code that did not require any lines. This made it concise and easy to follow. 
Lastly, correctness seem to be hit or miss. I was not able to get all three of the individual cases to cooperate with each other in the same function (begging the question about if registering three different functions would have been better). However, over three iterations, I was eventually able to get individual cases for all three. 

In addition to these main three cases, other basic conventions were followed. I was able to understand the readability and structure and the motivations behind implementing them. 

*nathan note*: elaborate more on the comparison between human and genAI code, strengths and weaknesses, 
- create csv with the results. 
- ask soraya about how to organize when chatgpt decided to make one large unit of a function. 
- maybe prompt more? see if there is more exploration to be had that could expand the story. 



== Exploratory Analysis and Discussion 


== Conclusion


== References


== Appendix