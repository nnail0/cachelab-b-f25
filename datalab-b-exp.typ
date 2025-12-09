#set text(font: "New Computer Modern")

#align(center)[
  #text(size: 20pt)[CS 341L Cachelab Part B Report \ ]
  #text(size: 16pt)[ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ Roxanne Lutz, Nathan Nail | Fall 2025 \ 8 December 2025 \ University of New Mexico]
]


== Summary

In this new era of technology, computer science cannot be discussed without a same-breath mention of AI. The technology is officially out of the box and can never be put back in. We would be remiss to ignore it. So, how exactly does it fit into the field and learning processes in general?

In this report, we will dive into a straightforward operation with a caveat. Solutions to the given problem must be generated through the old-fashioned way, so to speak. However, we open the door as well to generating solutions with the exciting new technology developed in recent years. 

With this experiment, we hope to explore the idea that genAI can be used in a successful direction to cut through a lot of the development time needed for a purely human-generated solution. It does, importantly, come at the cost of losing understanding in the process itself which can be beneficial in learning how to build solutions generally--ones which AI may not always be able to predictably build for us. 

== Introduction and Motivation

The assigned experiment focused on in this report is as follows.

*Optimize with and without genAI tools the trasposition of a matrix for 3 specific matrix sizes (_32x32_, _64x64_, and _61x67_) for a direct-access cache that has 32 sets and 32 bytes held per line.*

Primarily, the goal, in a tangible sense is to get both methods to reach under a specified boundary:
- _32x32_: misses < 300
- _64x64_: misses < 1300
- _61x67_: misses < 2000

The overarching motivating factor of the work done in this experiment is to highlight the strengths and weaknesses of both approaches to the problem. On one side, we will explore the lengthy process to develop code to meet this goal entirely from scratch without the benefit of genAI assistance. Likewise, we will dive into the idealistic code generation with genAI, discussing its benefits and drawbacks alike. With all pieces together, we will end by discussing the main differences in the approaches to the problem, focusing on the key takeaways from our results.

This report is organized into 5 sections beyond this Introduction.
+ Methods: General summary of methods used to generate both human and genAI code for transposition.
+ Data Collection: In depth discussion of the process in which human code was created, including failues. Likewise, in depth discussion of prompting attempts with genAI to generate code with the same goal.
+ Exploratory Analysis and Discussion: Discussion of results and primary differences in code approaches.
+ Conclusion: Concluding points of the report.
+ Appendix: Brief inclusion of documentation details about documents included with submission

== Methods

This project was primarily split into two natural halves between the two partners: 
+ One person's focus was upon, entirely from scratch, approaching the optimization by hand.
+ The other person's focus was on the generation of solutions with generative AI. 

The following parts are the methodologies used in generating potential solutions. To analyze the solutions, we will be using the provided cache simulation code to gather miss rate data to better compare and build up approches.

=== Human
The human generated code was developed in the following order:
+ _32x32_ optimization
+ _64x64_ optimization
+ _61x67_ optimization

The most useful approach to each of these problems was developed by the approach to the first. Picture/color representations of the set overlays on the matrics were generated to a degree in which the problematic miss rate areas were visibly simple to identify. Then, as will be discussed in the data collection portion of this report, solutions to mitigate these miss rates were developed based upon successful changes.

=== GenAI
The genAI code was developed with ChatGPT 5 mini (with reasoning), which is the version of ChatGPT offered on a trial basis to those who have an OpenAI account but do not pay for any other tier. The model opted to approach the code in a rather monolothic manner and generated code for all three cases to put into one function. 

When provided with code from the model, the code that was already in the `transpose-submit` function was directly and fully replaced, even if 1 or 2 out of the three ended up correct during the previous iteration. This was likely a suboptimal approach, but it demonstrates the deficiencies of ChatGPT's "memory". It likely would have been beneficial to split up the code generation into three different chunks to allow the model to analyze each case separately. The results of this method will be discussed later in the report. 

== Data Collection
The following section goes into depth on the processes of generating solutions, both human and genAI alike. 

=== Human

The following discussion of code generation will be presented from worst attempt to best attempt (or, the final submission that accomplishes a miss count under the minimum bound for each matrix). It should be noted that the attempts were not necessarily in a purely successful order, but will be presented as such for clarity.

The subsections as follows are the order, as mentioned, the human code was generated in overall. They will include, in context, a discussion of attempts made, what was learned from the failure of the attempt, leading to how the final optimized function was created.

==== _32x32_

As with all of the attempts made, the initial _32x32_ baseline miss rate was found through the simple `trans` function given. At worst, the number of misses was _1184_. A far-cry from where the miss bound needed to be at _< 300_, there was some exploring to be done on how the sets of the cache laid.

There are some pieces of key mathematics needed to understand how the sets of this direct-access cache lay and how we could best use it. With the numbers given, as discussed, we have _$S = 2^5 = 32$_ sets, and each set can hold _32 bytes_ (again, _$B = 2^5 = 32$_). So, we will end up holding _8 ints_ in a set, since

#set align(center)

_(8 ints) \* (4 bytes per int) = 32 bytes to a line/set_

#set align(left)

For the _32x32_, we can see a couple key patterns. First, the sets exactly fit within the dimensions, the first row being sets _0 - 3_, next row being sets _4 - 7_, and on. Further, the sets repeat their row overlay every _8 rows_. 
 
Therefore, when transposing, if we store a value in B and access in A, it is in the best interest in that cache to stay within those sets as much as possible. So, the first attempt past a baseline transposition attempts to make use of this fact and conducts a straightforward transpose procedure but within _8x8_ blocks. By staying within these _8x8_ blocks, we can make use of the _8 int_ values loaded into the cache lines as much as possible before moving on to the next _8x8_ blocks. 

This attempt drops the miss rate significantly, only having _344_ misses. From there, we need to analyze where the worst of the misses occur. We can see, by verification through trace files, that A and B can be treated as contiguous in memory. The use of this is that the sets of the cache overlap in the current access pattern specifically at the diagonal blocks (_rows/columns 0 - 7, 8 - 15, …, 24 - 31_). When we transpose these specific _8x8_ blocks, accessing A and storing in B, we are thrashing the sets heavily due to this overlap.

So, one way to address this is to load things preemptively, and allow one matrix full control, so to speak, over a set until it is ready to relinquish control to the other. To accomplish this, we will load a diagonal value of A into a temporary variable (for example, `temp = A[0][0]`). This allows A to claim control over the set overlaying this small block row (`A[0]` allows A control over _set 0_, following our example). Then, we allow B to have control over all the other sets, setting all values of B except for the diagonal (avoid `B[0][0]`, set `B[1..7][0] = A[0][1..7]`). This allows A to hit on all accesses in this iteration past the initial access. Then, when we are done allowing A to have control of a set, we hand over control to B, storing the temporary value grabbed at the beginning into the appropriate diagonal slot (`B[0][0] = temp`). 

We can add the following code as a special-case to our _8x8_ blocking technique.

```
if (i == j) { // if we're in a "diagonal block"
  for (ii = i; ii < i + blocksize; ii++) {
      temp = A[ii][ii]; // give A control over the row
      for (jj = j; jj < j + blocksize; jj++) {
          if (jj != ii) { // skip over all cols of B that will thrash A's row control
              B[jj][ii] = A[ii][jj];
          } // end if
      } // end loop
      B[ii][ii] = temp; // give B control over this set by ending with a diagonal set
  } // end loop
}
```

With this careful management of thrashing, we manage to plummet the miss rate below our boundary: *_288_* misses.

==== _64x64_

As with the _32x32_, we will start with a baseline of running the `trans` function to identify the worst-case miss rate, which comes in far higher than the prior square at _4724_ misses.

As an initial measure, since the _64x64_ is also equally divisible into _8x8_ blocks, we will attempt the same blocking mechanic as the _32x32_, negating the diagonal handling for now, to see the behavior untouched. This unfortunately has no tangible effect, having the exact same amount of misses as the baseline: _4724_. 

However, with blocking being a remarkably successful method in mitigating the miss rate, we can try changing the blocksize to see if any others improve. After some exploring, a blocksize of _4x4_ instead ended up having the best decrease in misses, dropping to _1892_.

From here, some closer analysis was required to get that number lower. There is a definitive difference in how the _64x64_ sets overlay the A and B matrices compared to the _32x32_. For the _32x32_, in each grouping of _ 8 columns_ (or _8 ints_ that will be loaded into a set line), every _8 rows_, a group of sets repeat. The _64x64_ has the same pattern, but with far more difficult consequences: in the same _8 column_ groupings, the set groups repeat instead every _4 rows_. This is the reason that the _8x8_ blocking alone does nothing of use; the general-case filling of B column-wise thrashes itself wildly within the _8x8_ block. Nevertheless, that is exactly why the _4x4_ block improves: B thrashing decreases considerably by at least hitting instead of missing on those smaller _4 int_ accesses per row before thrashing itself in the next iteration/block.

We still run into an efficiency issue: in the general-case (we will cover diagonal blocks in a moment), we are not making the best use of the numbers loaded into the cache line. When we utilize a _4x4_ block, we never use the second half of the set line when filling B column-wise. So, there must be an access pattern that minimizes misses between A and B both with filling blocks. Through many drawings, a mixture of the first two attempts was developed. First, we will overall still deal with _8x8_ blocks since the cache lines still load in _8 ints_ at a time, and we want to use that fact to our advantage. However, we will change how we fill the _8x8_ blocks by using smaller _4x4_'s within the larger block. We will focus on the access pattern of A: access the smaller _4x4_ sections of the _8x8_ block as a sideways-U-shape. Meaning, we access the block in A in the following order: upper-left _4x4_, upper-right _4x4_, bottom-right _4x4_, and bottom-left _4x4_. 

This has 2 benefits: (1) we can use all the _8 ints_ loaded into the set-lines of the top _4 rows_ on the A-block before allowing A to thrash itself on the bottom _4 rows_, using likewise all of those bottom _8 ints_ loaded up; (2) we can lessen the self-thrashing of storing in B-blocks since we are filling the _8x8_ as a rightside-up-U-shape, which admittedly will self-thrash twice, but overall will decrease evictions generally. Changing over all blocking to fill using this access pattern decreases the misses further to _1644_.

Now, on to the massive problem of the diagonal blocks. The thrashing that occurs at these diagonals is far worse than that of the _32x32_, as evidenced by the _8x8_ first attempt blocking having no benefit to speak of. Not only is there the typical A and B thrashing each other behavior, but now A and B are additionally thrashing themselves. Not even the U access pattern is enough to fix this problem. We need a unique solution to decrease the miss rate in these blocks specifically to even approach our goal of _< 1300_ misses.

So, the next attempt utilizes a convoluted loophole. We cannot modify the contents of A, but we can modify B in any way we wish. So, there is no reason we cannot use B as a conversion to get the diagonals to force them into being a general case block fill. 

The idea is that we can handle the diagonals first as a special case. First, we place the contents of the A-diagonal-block into the _8x8_ block next to the B-diagonal block (for the first 7 diagonal blocks, place to the next right _8x8_, for the last, place in the next left _8x8_). This effectively copies the problematic overlapping-set values into sets that will be guaranteed to not overlap with the B-diagonal-block sets. Next, use the U-shape access pattern to fill the B-diagonal blocks as we did in the prior attempt, but instead fill from the temporary storage block next door. This idea is successful, dropping our tally to _1412_ misses. So, this idea was in the correct direction, but needed improvement.

Instead of using the blocks next to each diagonal, we will instead use one consistent storage unit in B for the left half of diagonals, and then a different consistent unit for the right half of diagonals. The following choice of storage unit is somewhat arbitrary, but it has some specific benefits: use _rows 0 - 4, columns 48 - 63_ for the left half, and use _rows 60 - 63, columns 0 - 15_ for the right half. The benefit to choosing these spots and filling them with top _4_ and bottom _4_ rows from A-diagonal-blocks is that, past the initial miss on accessing these _8_ sets in B, we will hit every time after as we fill in each iteration from A. These spots in B, further, will not self-thrash when filling B due to them being to the left/right of each other and not above/below. Even better, we can access A row-wise when filling these storage areas, avoiding all except _4_ initial misses and _4_ self-thrashes. The same holds for B: we can now fill the B-diagonal-blocks row-wise with the needed corresponding values from the B storage units, minimizing to _4_ initial misses and _4_ self-thrashes in the diagonals. Despite being arbitrarily chosen, we can depend upon these spots for the left/right halves of the matrix to procedurally conduct these diagonal transpositions in a convoluted, but cache-optimized fashion. Then, we continue the general case U-shaped access pattern for all other _8x8_ blocks in the matrix as we did before. Although the implementation of this is far more difficult than the simplicity of `trans`, we finally get below the bound using this technique, arriving at *_1268_* misses.

Note that all code for these attempts can be found in `trans-human.c`. It will be kept there due to length of the functions, since even snippets of that code are many lines in length.

==== _61x67_

Yet again, we will consider the baseline miss rate by running the transposition functionality with the given typical trans function. This yields a base miss rate that is about as high as the _64x64_ baseline: _4424_.

The uniqueness of this size lies in its rectangular shape, no longer being the nicely consistent set overlay that the _32x32_ and _64x64_ exhibit. By drawing out some of the sets briefly assuming a start at address _0x0_ and A (_67x61_) and B (_61x67_) being contiguous, we can see that the sets repeat in patterns of _4 rows_ as did the _64x64_. However, due to the rectangular shape, we do not see the exact _8x8_ block issues as the _64x64_. The sets instead repeat in a diagonal pattern instead of straight vertical. This, as intimidating as it initially appears, ends up being a benefit over the squares in terms of meeting our bound; the sets do not overlap so horribly at the _8x8_ diagonal blocks. 

So, the first attempt made was to simply try blocking tactics that worked well with the other sizes. First, we will try to block _4x4_ (as the size allows within the rectangle), and fill with the straightforward blocking technique--as in, no U-shaped access patterns. We will leave diagonals alone since they do not align as consistently. That simple access pattern already works very well, yielding _2426_ misses. 

Since the usage of normal tactics appears to work well, it seemed worthwhile to ensure that the straightforward approach of an _8x8_ blocking (without the smaller _4x4_ access patterns) does not provide benefit. Upon running testing, a simple _8x8_ block actually does end up providing an even lower miss count at _2119_. 

With such a close number to the goal boundary of _< 2000_, this was likely the approach to toy with further. By simply changing the outer two block iterators from 

```
for (i = 0; i < N; i += blocksize) { // rows
    for (j = 0; j < N; j += blocksize) { // columns
```
to 

```
for (j = 0; j < M; j += blocksize) { // swapped outer loops to move B blocks rowwise
    for (i = 0; i < N; i += blocksize) { // A blocks col-wise
```

this effectively move the B storage blocks left-right row-wise and A access blocks up-down column-wise. This succeeds in getting the misses below the bound, coming in at a final *_1914_* misses. 

=== GenAI

For the genAI section of the code, ChatGPT 5 mini (with intelligence) was handed the entire PDF, in addition to some instructions provided to it: 

#block(
  fill: luma(230),
  inset: 8pt,
  radius: 4pt,
  "Hello, I am working on the CS:APP cache lab and was wondering how familiar you are with it. Given the assignment PDF, I was hoping that you could review it, confirm to me that you understand the goals of the assignment, and generate output that meets the criteria to the best of your ability.
"
)

From this, ChatGPT reiterated what it understood about the assignment and opted to handle all matrix sizes at the same time. To handle this, it uses if statements and calls to `return` in the case that the _32x32_ or _64x64_ case is being processed. The _61x67_ case is processed until the end of the function call. 

Below, we discuss:
- The results of each evolution of the code. 
- The strategy that the model employed at each setup.
- What ended up being the winning strategy for each case, despite the three cases not able to exist in harmony with each other.
- Future experiments to conduct with the code to see if improvements are made (i.e. what if, instead of one monolothic function call, there were three smaller function calls for each case?).

==== _32x32_

The 32x32 case, despite being the "simplest", was a case that was difficult for the model to score well on immediately. It took about four attempts to generate code that hit under _300_ misses. 
- First run: ran correctly, but was unable to reach the desired miss threshold at 344. 
- Second run: all three cases including _32x32_ were 0. This does not seem to be well-defined behavior and indicates that the model potentially generated code that broke the tester. 
- Third run: After some troubleshooting to deal with the previous run, this was the first iteration of the code to generate output that yielded *_288_* runs. This unfortunately came at the cost of breaking one of the other cases in code. 

In addition to succeeding with the _32x32_ case, the secondary goal was to remedy issues that were occurring with the final test case of _61x67_. Despite instruction to maintain the previously working case, some issues still managed to arise. This suggests that there were some side effects as the result of the "properly working" _64x64_ code that went under the miss rate.

The winning strategy for this case ended up being pretty simple, employing _8x8_ blocking and solving each block individually with the naive method. With less data to handle at each step, less cache misses were recorded.

==== _64x64_
This code followed a bit of a different story. The development that this code took further suggests that there may be some side effects with some of the variables being used, since we cannot currently provide a plausable explanation for what else could be happening; the model maintained the same structure and order in the code as it did previously, so some faults were unclear in source. 
- First run: invalid result. This code iteration produced suboptimal yet valid _32x32_ code and right-on-the-spot _61x67_ code, which was strange. Taking a quick glance at the code, I was never fully able to determine the cause of the invalidity. 
- Second run: Like with other versions, the code that was generated produced 0 misses, which is effectively impossible and indicates that some sort of issue occurred with this iteration. ChatGPT was then prompted to revisit this code and ensure that the setup was being conducted properly. 
- Third run: Like with the _32x32_ case, the _64x64_ code hit well under the miss target of _1300_ and netted *_1180_* misses. This came at the cost of breaking the _61x67_ test case. 

Runs beyond Run 2 were to try and get a round of code that would integrate the successes of these two cases with the first-run success of _61x67_; this proved unsuccessful. 

The model's winning strategy proved to be somewhat successful, but after looking at what it took to hit the target miss rate in the human-generated code, this makes sense. From what we can understand of the code, it followed three major phases: 
+ Handled the first four rows of each _8x8_ block in a standard fashion, starting from the top left. 
+ Switched to a different strategy that involved breaking it down into further _4x4_ blocks and transposing them in a specific order. This seemed to handle two or three of the four _4x4_ blocks that the the code partitioned off. This was the only section of code out of the three cases that used the eight extra variables alotted to it. 
+ Finished off the final _4x4_ block with a more standard transpose techniques. 

==== _61x67_
This test case ended up being one of the simplest, with the best results happening with very few lines of code and with the fewest number of attempts. 

- First run - instantly met the miss rate criteria. This code managed to just squeeze under the miss rate of _2000_ at *_1993_* misses with about 2 dozen lines of code. 
- Second run - like with all of the others, this was invalid due to issues with the code. 
- Third run - This was the first run to nail the other two cases, but this, somehow, came at breaking the _61x67_ case despite explicitly prompting the model to keep the original working code. We have yet to see whether the reason that the code broke was due to actual changes in the code or due to side effects from previous runs. 

The winning strategy for the _61x67_ case was fairly simple and adopted most of the code from the standard _8x8_ blocking strategy. The main change involved extra checks on the outer loops (given the irregular size) to handle not writing too much or too little on either of the dimensions. The inner loops employed the naive implementation. 

==== Second Attempt: Three Separate functions
As mentioned, there was an intimation that there were some side effects occurring under the hood as a result of having all three cases in the same function. As a result, the model was prompted to make another attempt. This time, it was given explicit directions to split the code into three separate cases. This ended up being the solution. 

Upon closer inspection, it appears that ChatGPT added in some corrections to the _32x32_ code, which makes begs more questions about where exactly the problem was happening before. Regardless, it appears that the model was able to reflect on its own mistakes from a previous conversation and find what went wrong. This could point to the previously mentioned "short-term memory" issues that that were mentioned earlier, or it could be something different. 

==== Code Quality
Some important aspects in measuring code quality involve legibility, conciseness, and correctness. In all, it looks like the model was able to produce code that that was mostly followable. 

For the _64x64_ case, the code was not terribly concise, which made it difficult to parse. However, it was able to generate a satisfactory result. The other cases could be distilled down to simple code that did not require any lines. This made it concise and easy to follow. 

Lastly, correctness seem to be hit or miss. We were not able to get all three of the individual cases to cooperate with each other in the same function, begging the question about if registering three different functions would have been better. However, over three iterations, we were eventually able to get individual cases for all three. 

In addition to these main three cases, other basic conventions were followed. The code itself was readable and structurally legible, and the motivations behind implementing them were comprehendible. 

*nathan note*: elaborate more on the comparison between human and genAI code, strengths and weaknesses, 
- create csv with the results. 
- ask soraya about how to organize when chatgpt decided to make one large unit of a function. 
- maybe prompt more? see if there is more exploration to be had that could expand the story. 

== Exploratory Analysis and Discussion 

=== Results

Overall, we were able to generate successful under-bound solutions both on the human and genAI side of code generation. Following is the comparsion of the best results from both sides.

#set align(center)

#table(
  columns: (auto, 1fr, 1fr),
  inset: 10pt,
  align: center,
  table.header(
    [*Sizes*], [*Human Best*], [*GenAI Best*],
  ),
  [*_32x32_*],
  [288],
  [288],
  [*_64x64_*],
  [1268],
  [1180],
  [*_61x67_*],
  [1914],
  [1993]
)

#set align(left)

As we can see from the above, the _32x32_ best records met exactly. From the code generated on both ends, both human and genAI came to the same technique of using a temporary variable to store the diagonal value. Further, there were approximately the same number of attempts made to come to that conclusion between the two--human took 2 attempts beyond the `trans` function baseline, and genAI took until the 3rd prompting. 

The _64x64_ is likely the most interesting to look at the differences of. From the human side, it was by far the most difficult to analyze, requiring much slow analysis of the set patterns and creativity in generating a solution. It also took the most failures of all the matrices, which meant days of attempts being concocted until the final success was met. 

Even after all that, genAI was able to produce a better solutions by about _90_ misses. Further, it only took 3 attempts of prompting to get a valid, under the target bound solution, which is impressive compared to the pains it took to generate something without being able to use it.

Finally, the _61x67_ was however beat by the human by about _80_ misses. The two methods used are slightly different in nature, as will be discussed below, but one strategy was clearly still slightly better than the other. However, given that both solutions are under the goal boundary, each are valid in terms of minimizing the misses.

=== Code Generation Comparisons

The main differences in human and genAI code appear to lie primarily in 
+ Time taken to generate solutions.
+ Understandibility of the solution.
+ Ability to build up the solution methodically.

==== Time

Overall the human generated solutions took around a week to develop to their current best miss rates. A bulk of this time was purely in initially beginning by toying around with code without any real intelligent goals in mind. This provided enough failure to start actually attempting the optimization in a way that was based on the set layout knowledge (as mentioned, hand drawing the patterns or using Excel sheets to create set color fills was very helpful). Then, the building of each attempt, which was built upon the prior failure, involved at times complex implementation details that had to be tested for correctness locally, only to be then tested for miss rate data.

All of this together, culminating to a week's worth of frustration, did however come together into a deeper undestanding of the cache mechanisms and a layer of creative problem solving in this context that is a worthwhile skill for real-world applications. 

However, that is not to say that genAI did not have it's uses here. The solutions that genAI, in spite of strange behavior in the initial prompts, took only a day to piece together. In terms of a real-life consequence, a workplace would likely perfer the inclusion of this tool to see if it can cut down the time taken on coming up with a solution to save the most important asset: money. Using this tool can be tremendously useful, then, freeing up time spent on the grunt work and providing a usable solution. This approach could be used postively to generate one or more optimized solutions on which to step in an improve as needed, cutting through the time to develop those initial failures to better focus on a final fine-tuned solution.

==== Understandibility

We nevertheless must discuss understandibility in two contexts: (1) Understanding the code itself in terms of legibility and conciseness and (2) understanding why the code works.

On first understanding the code from a technical perspective, the human code submitted has the benefit of the comments being as verbose as needed--as much for the developer as for the reader. Considering at least the _64x64_ is an admittedly long and somewhat convoluted solution for the human code, these comments are necessary to even begin to piece together what we are doing mechanically. 

On the topic of convlusion, the genAI solutions have the opposing perk of having the same or less number lines overall. This is a huge improvement over the human code as most notable in the _64x64_ solution, which comes into about _133_ lines, as opposed to the _70_ line solution from ChatGPT that even performed better than the human solution. Generally, the shorter solution is easier to digest, along with straightforward commenting to interpret the intention of the code. 

This topic naturally leads into discussing the second point: understanding why the code works. We are able to see the general mechanics of the genAI code functionality as specified in comments. A drawback, nevertheless, is in missing why this code works. 

When discussing the human code development process, and walking through all failures, we can see the exact train of thought that led to the final solution, starting with the most basic algorithm to the best. This is exactly what we miss in ChatGPT code. There has to be further prompting in order to get the explanation of why exactly this works. Even then, there has to be an element of wariness when reading the explanation due to the potential hallucinations of the black-box. The human instead has an explanation that can be built up from failures, learning, and data to support the next step justification, making the process more understandable at a deeper level to both developer and reader. Additionally, the process can be read by others to improve upon, since others may see some sort of successful alternate approach from the development process that the original developer did not recognize. In all, the end answer is not always the only useful piece of the solution process.

==== Methodical Development

As evidenced a discussed prior, ChatGPT's greatest downfall in this process was the ability for it to ignore prompting, break its own code, and output unpredictably. This was, as seen by the secondary prompting technique of generating methods versus generating solutions all in one fixing many of the issues, a fault of the prompting. Just as people do, ChatGPT performed far better in terms of generating a functional solution by forcing it to separate the problems into separate but related pieces. 

Naturally, the human code was generated starting with the perceived simplest matrix first (_32x32_) and working towards the harder cases (_64x64_, _61x67_). This methodical development made it simpler to use known successful techniques in the harder cases and then tailor the solutions according to the unique problem needs. We will therefore hypothesize that this would have been a similiarly better tactic in prompting genAI to produce a faster success. Further trials should be conducted on this strategy to verify, but so far this experiment suggests that this is the case.

Finally, as easy as it is to cast stones at genAI for having unpredictable side-effects and breaking its own code, there were admittedly many times that the human code did the same thing while testing functionality. Typically nothing was being broken outside of a specific method, but the potential for mistakes in complicated solution implementation was very high during human development. So, with more focused prompting, genAI shaves much of that headache successfully.

== Conclusion

To put all pieces together, human and genAI code came in quite close in performance, either being exactly the same in performance or one ever slightly beating the other. However, both approaches to the problem resulted in a solution under the desired boundary. So finally, we must ask: which approach is better?

The term "better" here can have different meanings, and it depends on the goal of the exercise. As we have seen, genAI can produce a solution in a few moments that is better than one that took a human days to develop from scratch. From a perspective on time saving, genAI is absolutely a tool that should be utilized to this end. As long as the tool is used appropriately and consciously, such as prompting in small pieces to not overwhelm the model into clashing with itself and being aware of unpredictability, this can be an incredible starting point for solving a problem, or improving upon a decent solution. 

But the term "better" can also mean the process of learning how to think in terms of the problem: to learn the intricacies and nuances of caches, how to use knowledge of the cache to improve upon failures, and to truly understand how to work within these confines. Although genAI can a quick, perfectly acceptable answer, the learning process is often far stronger when compounded by failure--something that genAI takes out of the equation entirely.

In conclusion, both approaches had their benefits and drawbacks. Depending on the goal in mind, it is up to personal reponsibility to identify the appropriate usage of this tool given the context.

== Appendix

=== Documentation Details

Some notes on formatting that are likely useful for the reader.

- `trans_human.c`: All methods to yield results discussed in report, as formatted for enabling running of `./test-trans` and `driver.py`. The attempt numbers correspond to the order the attempt is discussed in the report. When running `./test-trans`, all of these attempts will run for the given size. Feel free to comment out the registration of any of these in `registerFunctions()`. `transpose_submit()` will run the best for each size case.
- `ai_trans.c`: The original `trans.c` file modified and filled in with output produced by ChatGPT. The code in this file is the most recent version containing three spearate function calls. 
- `ai_trans_old.c`: This is an older version of the code that ChatGPT generated, where all three cases were handled inside of one function. Implementing the code in this manner led to issues, which necessitated a different approach. 
- `trans_human_results.csv`: Results as gathered from `./test-trans` functionality for each size, corresponding to function names in `trans_human.c`. Formatted in the order: size, function name, hits, misses, evictions.
- `ai_trans_results.csv`: Results gathered from running `driver.py` and formatted 
