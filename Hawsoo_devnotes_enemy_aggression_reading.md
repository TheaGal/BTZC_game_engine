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


3. CPU detects PC entered close-range.
    - CPU will interpret this as aggression, and perform an attack.


4. CPU detects PC running away (distance keeps growing for x-seconds).
    > This may be finicky due to CPU running away to do a charge-in attack.

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


## Glossary

CPU - Computer. NPC character (that in this context is insinuated to be hostile towards the player character).
PC - Player character. The character the User has direct control over.
