Select `GAME_WORLD_ASSIGNMENT 3` at the top of [Go.cpp](source/Go.cpp), then rebuild GameMain.

# U03: let's walk through the game

This assignment is about one simple question: **how does a guard find out what
is happening around it?**

We try two ways. One guard asks for information. Another gets a message when
something changes. They should follow the same rules, even though they get
their information differently.

That's the main idea. You don't need to know every engine file to follow it.
Let's start with what you see on screen.

## What happens in the game?

We have one player, three computers, and four guards.

Left-click somewhere and the player walks there. Walk within 75 pixels of a
computer and hacking starts automatically. Walk away and it stops. There is
no hacking button, progress bar, or timer.

A computer turns green while it is being hacked. After you leave, the latest
computer you tried to hack turns yellow. The other computers stay white.

The guards have two rules:

- A smart guard remembers the last computer you tried to hack. It keeps going
  there even after you leave.
- A stupid guard only follows the computer you are hacking right now.
  When you leave, it stops where it is.

There are two of each so we can compare asking for information with receiving
messages. Their names on screen tell you which is which.

## Who does what in the code?

Think of an `Actor` as the body and a `Controller` as its brain.

The brain chooses where to go. The body handles moving and drawing its picture.
We already had this setup, so the assignment reuses it.

| File | Its job, in plain words |
| --- | --- |
| [Go.cpp](source/Go.cpp) | Starts the engine and keeps the game running. |
| [GameWorld03_WorldInterface.cpp](source/Worlds/03_WorldInterface/GameWorld03_WorldInterface.cpp) | Creates the scene and checks which computer is being hacked. |
| [PlayerController.cpp](source/Controllers/Steering/PlayerController.cpp) | Turns a mouse click into a place to walk to. |
| [WorldInterfacingControllers.cpp](source/Controllers/WorldInterface/WorldInterfacingControllers.cpp) | Contains the four guard brains. |
| [AIPollingStation.cpp](source/Worlds/03_WorldInterface/AIPollingStation.cpp) | Answers the polling guards' questions. |
| [AIEventManager.h](source/Worlds/03_WorldInterface/AIEventManager.h) | Names the hacking messages and what they carry. |
| [EventManager.hpp](../../CommonUtilities/include/Events/EventManager.hpp) | Delivers messages to the objects that signed up for them. |
| [Actor.cpp](source/Actors/Actor.cpp) | Moves an actor and draws its sprite, meaning its picture. |

If you're opening the code for the first time, start with `GameWorld03_WorldInterface.cpp`,
then the guard controllers. The section comments should help you find your way.

## Polling means asking

Imagine a guard asking, "Which computer is the player hacking now?"

That is polling. The guard asks `AIPollingStation`, not `GameWorld03_WorldInterface` directly.
The station reads the information stored in the world and gives it back.

The stupid polling guard asks:

```cpp
myTargetComputer = myPollingStation.GetCurrentlyHackedComputer();
```

The smart polling guard asks:

```cpp
myTargetComputer = myPollingStation.GetLatestAttemptedComputer();
```

That difference matters. "Currently hacked" becomes none when you leave.
"Latest attempted" still remembers the computer you left behind.

### Why does the station remember a frame number?

A frame is one turn through the game loop: update things, then draw them.
`GameWorld03_WorldInterface` counts these turns: frame 1, frame 2, frame 3, and so on.

Suppose we are on frame 12:

1. Someone asks for the current computer. The station reads the world, saves
   the answer, and writes down "I got this answer on frame 12."
2. Someone asks again. It is still frame 12, so the station gives back the
   answer it already saved.
3. On frame 13, the first person who asks gets a freshly read answer.

That saved answer is called a *cache*. Nothing more mysterious than that.

Each question has its own saved answer and frame number. If nobody asks a
question during a frame, its answer isn't refreshed.

The frame number is not a limit on how long a frame may take. We don't have
that extra limit anymore.

## Events mean getting a message

Instead of asking every frame, an event guard says, "Tell me when hacking starts."

It signs up with `AIEventManager`. When `GameWorld03_WorldInterface` notices that hacking has
started, the manager calls the guard's receiving function.

We have two messages:

- `PlayerStartedHackingEvent`: the player started hacking this computer.
- `PlayerStoppedHackingEvent`: the player stopped hacking this computer.

The message points to the computer it is about. It does not create another computer.

If you see the word *callback*, it just means the function we want the manager
to call when a message arrives. `OnPlayerStartedHacking` is one example.

The smart event guard listens only for start messages. It remembers the
computer and keeps that destination when the player leaves.

The stupid event guard listens for both messages. A start gives it a destination.
A stop clears the destination, so it stops moving on its next update.

### Why not send "the player is hacking" every frame?

Because nothing new happened. Once we have told a guard that hacking started,
we only need to tell it when that changes.

Stay beside computer 1 for ten seconds and we still send just one start message.
Leave and we send one stop message. Come back and that is a new start.

If the current computer changes directly to another computer, we send stop for
the old one, then start for the new one.

Messages are delivered straight away; they don't wait in a queue. Receiving
functions just update the guards' destinations. They must not add or remove
listeners while a message is being delivered.

See [EVENTS.md](../../CommonUtilities/EVENTS.md) for a separate example of signing up for messages.

## Putting the four guards side by side

| Guard | How it finds out | What it follows |
| --- | --- | --- |
| `SmartGuardPollController` | Asks the station each frame | The latest attempted computer |
| `SmartGuardEventController` | Receives a start message | The latest attempted computer |
| `StupidGuardPollController` | Asks the station each frame | The computer being hacked now |
| `StupidGuardEventController` | Receives start and stop messages | The computer being hacked now |

The two smart guards should follow the same rule. So should the two stupid guards.
They start in different places, though, so they don't have to arrive at the same
time. Guards can overlap when they reach the same computer.

## What happens during one frame?

The order in `GameWorld03_WorldInterface::Update` is important:

1. Increase the frame number.
2. Move the player.
3. Check whether hacking changed. Send any needed start or stop messages.
4. Update the guards. Polling guards ask their questions here; event guards
   already received any new messages in step 3.
5. Update the debug panel. The game loop then draws the scene.

Both kinds of guard react to the player's new position. Neither is working
with information from the previous frame.

## A few movement details that can look confusing

The controllers choose a direction and speed. `Actor` changes its actual
movement towards that choice, rather than jumping straight to the destination.

The player and guards slow down within 80 pixels of their destination. Within
4 pixels, they settle exactly at the destination and stop. That stops them
wobbling around a point they have basically already reached.

`aTimeDelta` means the time that passed since the previous frame. The actors
receive that time directly, with no extra frame-length cap.

### What is DistanceSqr doing?

It leaves out the final square root normally used to calculate distance.
For example, being 3 pixels across and 4 pixels up gives:

```text
Distance squared: 3 × 3 + 4 × 4 = 25
Normal distance: square root of 25 = 5 pixels
```

To check whether that is within 10 pixels, either comparison gives the same answer:

```text
Normal distance:   5 <= 10       -> yes
Distance squared: 25 <= 10 × 10  -> yes
```

Our hacking check uses the second version, comparing against `75 × 75`.
Skipping the square root saves a little work. It is useful when checking lots
of distances, but makes very little difference with three computers.

### Why do we flip the mouse's Y position?

The mouse counts down from the top of the window. The game counts up from the
bottom. We flip Y so clicking near the top moves the player near the top.
We also account for the window and drawing area being different sizes.

## What does nullptr mean here?

A computer pointer is a way to refer to a computer that already exists.
`nullptr` means "no computer."

When the stupid guard's destination becomes `nullptr`, it has nowhere to go
and stops. Changing that pointer does not create or delete a computer.

`GameWorld03_WorldInterface` keeps the computers and managers. `ActorManager` keeps the player
and guards. Each actor keeps its controller. When the game closes, guards stop
listening before their event manager is destroyed. We don't want the manager
trying to call a guard that no longer exists.

## Try it yourself

Build `GameMain` from `Game.sln` using Debug or Release. Start the game with
`Bin` as its working directory; that's where it expects to find its settings.

From an x64 Visual Studio developer command prompt in the repo folder:

```bat
msbuild Local\GameMain.vcxproj /m /p:Configuration=Release /p:Platform=x64
cd Bin
GameMain_Release.exe
```

Then try this:

1. Walk up to computer 1. All four guards should head towards it.
2. Leave before they arrive. The smart guards keep going; the stupid guards stop.
3. Walk up to computer 2. All four guards now head towards computer 2.
4. Stay there for a bit. You should not keep getting new start events.

The small ImGui panel, meaning the debug window, shows the current and latest
computer. Zero means none. Open its sections when you need more information:

- **Visuals** lets you show hacking-range circles and destination lines.
- **Events and polling** shows message counts and polling counts. It also lets
  you print those updates to the console.

For polling, *requests* means questions asked and *refreshes* means new answers
fetched. More questions in the same frame must not mean more refreshes for that answer.

There are no separate game tests anymore. The game uses the normal engine and
renderer, meaning the part that draws it. It does not have a mode that runs
without them. Retail builds leave out the debug panel.

## Where this comes from

The rules are in [U03_World_Interfacing.pdf](../../Doc/U03_World_Interfacing.pdf).
The setup follows [F04_World_Interfacing.pdf](../../Doc/F04_World_Interfacing.pdf),
especially the saved-frame example on page 8 and the diagram on page 13.

Some earlier steering controllers are still in the repo, but `GameWorld03_WorldInterface` now
sets up this assignment, not the old flocking scene.

We also repaired some build setup: missing FBX libraries, generated DirectXTex
shader files, and unnecessary library links. Those are engine/build details,
not extra guard rules. You can leave them alone while learning U03.
