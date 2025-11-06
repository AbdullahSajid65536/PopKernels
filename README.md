# PopKernels

A small minigame I made in C to experiment with POSIX threads, atomic operations, signal handling and numerical methods.

The game itself is straightforward, choose a popcorn bag size and a microwave timer. pop=win, not pop=lose, burn=mega lose. You may also cancel the timer to end the game for a penalty. 

3 loss conditions and one win condition... sounds pretty fair.

Needs a lot of balancing and has some bug fixes that I will address eventually... (About time I post something to my Github anyways...)

### IMPORTANT NOTE: THIS MINIGAME IS ONLY SUPPORTED FOR LINUX SYSTEMS. For windows users I recommend using WSL/Docker or a VM etc. to run it. As for Mac users, since MacOS is POSIX compliant I think it may work but I cannot guarantee it.

For more info, the documentation is included as part of the code, but I will paste it here verbatim.

## Welcome to Pop The Kernels! A multi-threaded minigame (pretend I inserted popcorn.jpg here)
The game starts with a bag of popcorn kernels being generated according to your specified bag size, and you specifying a set period of time for a microwave to cook them up.

Inputs: <Microwave time (in seconds)> <Bag Size (Specify one of these flags): O|XS|S|M|L|XL|X(2...4)L>

Flags Breakdown: 


  O: One Kernel Only.
  
  XS: Extra Small - 8 Kernels.
  
  S: Small - 16 Kernels.
  
  M: Medium - 32 Kernels.
  
  L: Large - 64 Kernels.
  
  XL: Extra Large - 128 Kernels... and so on up to X4L.
  
  
Example Input (Command Line): ./popkernels 120 X3L - Runs for 120 seconds with a XXXL bag (512 kernels!)."
_*Note that custom bag sizes are not supported._

# How does this work?
All popcorn kernels use a fixed value N and store two arrays in a struct based on this value:


1- An NxN matrix (A) - 2D Array

2- An N-d vector (b) - 1D Array


Now a thread performs calculations to reverse engineer a solution to this matrix vector pair and all the elements of the solution vector are summed together to generate a score.
Once this is done the thread returns a pop sound.

When the timer runs out, all the unpopped kernels will negatively affect your score using the unsolved vector b.

So.... just spam the biggest value for the microwave and call it a day ig

_(Do note that burnt popcorn penalize you even more than what you earn per piece :D)_

## Now that you know the rules, go ahead and try it out!

I welcome any feedback for this project as a LOT of balancing is needed to make it fair, and a lot of bug fixing to make it playable.

To Do List (so far):

1- Fix partial pivoting:
  -div by zero error resulting in infs
  -issue with numerous i,j etc. orderings because I lost track

2- Improve thread handling for the burner thread with cancellations etc.
