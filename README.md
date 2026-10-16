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

My Amazon ebook and paperback "How to Write a 64-bit Chess Neural Network" will be available soon.
It includes a solid introduction to neural networks in general. 
