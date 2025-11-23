<h3>Bills-simple-chess-neural-network</h3> 
This is a simple chess engine that uses a neural network to select it's moves.

The network is initially untrained, but trains quickly, owing to it's small size.

It has three layers, including one hidden layer.

It is written entirely in C/C++. No external libraries are needed.

I have attempted to make the code as simple as possible.

The engine features:
<ul>
<li>Move and attack generation.
<li>An internal opening book.
<li>Mate and draw detection, including draws by stalemate, repetition, the 50 move reduction of material.
<li>Unsupervised training with self-play. No external data files are needed.  
<li>Forward and backpropagation.
<li>Weights may be saved and later loaded.
<li>.fen files may be loaded.
</ul>

There is an eBook explaining the program at https://www.amazon.com/dp/B0FPRJN8MW 
It includes a solid introduction to neural networks in general. 
