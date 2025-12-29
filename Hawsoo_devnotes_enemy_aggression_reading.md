# Hawsoo dev notes for: Enemy Aggression Reading.

So it appears that there's a bit of an issue "measuring" aggression.



I guess these are the cases in battle I could think of:

1. CPU is low in health or high in posture buildup meter. The CPU is going to lose.
    - CPU will either (1) become desparate and do a super aggressive move on the PC, or (2) become defensive and try to get to a position where they can recover.
    > Ig there's combos too, like in KUSR, great shinobi owl will spray poison to create a barrier and then use that position to attempt to recover.


    > I guess Case 1 would be right after PC is being aggressive, since it would be an attack or deflect? Mmmm ok maybe not huh.

    > For Case 1, it probably should get triggered by low health, not necessarily based off some kind of "desperation" arbitrary value.


2. CPU is high in posture buildup meter, and PC is not aggressive.
    - CPU will rest by not attacking, essentially a 様子見.
    > Mmmm for this, I think just not paying attention to the posture buildup is fine.

    > So only if PC is not aggressive, then CPU has the ability to decide to idle, which helps lower posture buildup meter.


3. CPU detects PC entered close-range.
    - CPU will interpret this as aggression, and perform an attack.

    > For case 3 and case 4, there should just be a "activate close-range" distance (`acrd`), and then a "deactivate close-range" distance (`dcrd`).
        > `dcrd > acrd` should be true always (and `acrd` and `dcrd` should not be close together), so that there's a solid difference between the distances, and there's deadzone instead of being fickle.


4. CPU detects PC running away (distance keeps growing for x-seconds).
    > This may be finicky due to CPU running away to do a charge-in attack.
    > Maybe just have a certain kind of attack that only happens when really really far away?
        > Thinking maybe moreso if this is a boss kind of thing.
        > For ranged enemies, they could pull out their ranged weapon and shoot.


5. CPU detects PC is attacking.
    - Attempts to block (guard/parry) the attack, then attempt to do a counter attack.
        - ~~Won't attempt to block unless `dot()` of PC and CPU facing angle are close to `-1.0`.~~
            > ~~This is the same requirement for PC to block CPU's attacks so ig that makes sense!~~
            > Okay, CPU will rely on a trigger to attempt the block.

    - In order to detect when to do a block, enable a trigger for 1 tick to signal to do the block.
        > Probably have to time when to do the signal depending on the attack?
        - And then if CPU's AFA data allows for ready-parry action, then will do it.
        - The trigger is just a big capsule in front of the PC model that gets switched on-off,
        - but for a shuriken-like thing it can be a capsule left on that runs ahead of the actual collider.
    
    - Actually, instead of having the collider type, just comparing the flat dot product (zero out Y) over some kind of broadcast event in the AFA that gets the message of origin,direction is attacking right now would be great. (So then direction and distance can get compared instead).


6. CPU detects PC is using consumable (e.g. healing).
    - Attempts to get an attack in if programmed to respond to this.


## AI combat queuing.

Basically having an attack queue that fills up when aggression is built up is the key here.

And then (similar to KUSR animation events) have a jump table to transition to another animation when there's a combo attack or an attack or a step or a move queued up (in that order of priority btw).
- @NOTE: Just pop off the front anim to transition to.

For getting hurt, there are 3 types:
- Heavy hurt (anim change)
    - This is the default.
- Light hurt (no anim change. Use particles and/or addive anim (if have the time for that))
    - This will happen if CPU/PC is doing a special attack that will not be interrupted.
        - The region will be marked with "Use light hurt".
        - @NOTE: "superarmor break hurt" will interrupt this, however.
- Superarmor break hurt (anim change and long. Could also trigger more aggressive attack anim).
    - This happens when health decreases a certain amount.
- (EXTRA TYPE) Posture break (anim change)
    - This happens when all health is depleted or posture buildup bar fills up.

NOTE: For the PC as well, there should be a combo attack, attack, step, move queue as well as for the CPU.
- Except for combo attacks not really being something that happens for PC.


## Glossary

CPU - Computer. NPC character (that in this context is insinuated to be hostile towards the player character).
PC - Player character. The character the User has direct control over.
