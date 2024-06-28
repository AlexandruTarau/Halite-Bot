# Halite Bot - Conquerors, Providers, and Freelancers

## Game Description
Halite is a multiplayer strategy game where players compete to gather the most resources (called halite) on a 2D grid map. Players control ships that can move, gather halite, and deposit it in their base.

## Bot Description
This bot employs a strategic approach using three types of ships: Conquerors, Providers, and Freelancers.

### Conquerors
- Conquerors are specialized ships that aim to expand territory efficiently.
- They utilize a Dijkstra-based algorithm to identify and conquer the "best" areas within a specific range.
- Once a valuable area is detected, Conquerors prioritize moving towards and capturing it, aiming to secure critical resource-rich zones.

### Providers
- Providers support the Conquerors by transporting resources back to the base.
- They optimize resource gathering and delivery routes to ensure efficient halite collection.
- Providers play a crucial role in sustaining and enhancing the overall resource acquisition strategy of the bot.

### Freelancers
- Freelancers operate independently to maximize resource gathering efficiency.
- Using a Greedy algorithm, Freelancers seek out and occupy the "best" cells within their vicinity.
- This autonomy allows them to cover more ground and gather halite efficiently from unclaimed or less contested areas on the map.

## Bot Strategy
- The bot's strategy revolves around coordinated efforts between Conquerors expanding territory, Providers optimizing resource transport, and Freelancers maximizing resource collection.
- By leveraging these specialized roles and algorithms, the bot aims to outmaneuver opponents and dominate the resource-rich areas of the map.

## Usage
- To deploy this bot:
  1. Clone the repository.
  2. Ensure you have the necessary dependencies installed as per the bot's instructions.
  3. Run the bot within the Halite environment and observe its performance against other players.
