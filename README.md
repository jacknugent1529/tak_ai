# Tak Board Game AI (In Progress)

AI agent for the board game [Tak](https://en.wikipedia.org/wiki/Tak_(game)). The algorithm is trained based on Monte Carlo tree search with neural networks for policy and evaluation functions. The primary resources used were [TTIC 31230 notes](https://mcallester.github.io/ttic-31230/).

## Organization

### MCTS Folder
This contains code for running the simulation. The game logic and simulation code is written in c++. It loads a pytorch model (compiled into TorchScript) which is used for inference. Simulations are run using the `takMCTS` executable.

This also contains a playable TUI in the `takTUI` binary. This can be played with 2 players, or against a bot. The `--model` flag to specify the bot expects a TorchScript model file to be specified.

### Model Folder
This contains code for training the model. The model's architecture is CNN-based. The policy and value networks share parameters for several layers.
