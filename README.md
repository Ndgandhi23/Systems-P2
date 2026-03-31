Logic behind code and our implementation of the project:
--------------------------------------------------------

Collection Phase
----------------
- In the collection phase, we use a method similar to Depth-First Search where rather than utilizing a set to store all directories and then going through that to continue traversing, as soon as we hit a directory we explore it recursively and then return back to the head
- We created many structs that have their own purposes:
    - WordNode is used to keep track of a word and its frequency per file where each file is given its own list of WordNodes
    - WFD is used to represent the head of the list of WordNodes, per file, and also keeps track of the total number of words in that file
    - FileArr is the data structure we used to keep track of all of the files we are working with
    - Comparison stores the compared data results between two files, therefore each pair of files is assigned a Comparison
- When recursing through directories and files, we use stats to see whether it is a directory or a file, and then proceed with recursing
- We set the suffix to be ".txt" and coded with the assumption that although the arguments the user provides do not have to end in ".txt", the files we find while traversing, not the ones given, end with a ".txt" in their names. Also wanted to note that the files the user provides do not end in ".txt" explicitly but are also stored in files as just that name without the ".txt" following it.
    - for example: if the user enter foo, foo is stored as foo in the files NOT foo.txt, although it is a ".txt" file
- We also tokenize each file to make sure we are storing words correctly. For example, words are allowed to have letters, numbers, and hyphens. All of punctutation is ignored. In addition to this, spaces are the only thing that can create a new word and all letters are set to lower case.
    - for example: "Can't" and "cant" are treated as the same word.
- If the user provides a file that begins with "." in the input, the file prints an error message
    - for example: ./compare testA .testA will print an error message
    - for example: ./compare testA test_dir/.testA will print an error message
- Over all structure:
    - FileArr keeps track of all files
    - each file is assigned its own list of Wordnodes using WFD


Analysis Phase
--------------
- In order to calculate the mean frequency of a word in two files we used our data structures that stored WordNode and were able to get the frequencies for each word and then perform the calculation
- In order to calculate KLD, we use the same information as we did for calculating the mean frequency and the value itself
- In order to calculate JSD, we required the KLDs calculated before and stored the value for the JSD in a Comparison struct so that it can later be sorted for the output
- When outputting, we sort the list of Comparisons, which contains the JSDs, by getting the total words for the pair of files.
- We also used a nested for loop for creating Comparisons between each pair of files, to ensure no repeats of pairs
- We also stored these Comparisons within a list