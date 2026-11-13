<h3>Deep Basic</h3> 
This is a simple chess engine that uses a neural network to select it's moves.

The network is initially untrained, but trains quickly, owing to it's small size.

It has three layers, including one hidden layer.

It is written entirely in C/C++. No external libraries are needed.

I have attempted to make the code as simple as possible.

I have recompiled the program as 64-bit. The extra speed helped speed up training. 

The engine features:
<ul>
<li>Move and attack generation.
<li>An internal opening book.
<li>Mate and draw detection, including draws by stalemate, repetition, the 50 move reduction of material.
<li>Training with self-play. No external data files are needed.  
<li>Forward and backpropagation.
<li>Weights may be saved and later loaded.
<li>FEN files may be loaded.
<li>Winboard compatibility which enables it to run with Arena. Arena is a popular program for running chess engines.
</ul>

This was created using MS Visual Studio 2019.

<ul>
<li>MS Visual Studio will need to be installed.
<li>Create a new folder where you want to extract the project.
<li>Select the Code Button and select <b>Download zip</b>.
<li>Extract the zip file into the new folder.
<li>Open the solution file.
<li>Build and Run.
</ul>

My Amazon ebook and paperback "How to Write a Chess Neural Network" is now available.
It includes a solid introduction to neural networks in general. 

https://www.amazon.com/dp/B0HNHX1B2J?lv=shuf&channelId=500&plpRedirect=mhFallback&th=1
