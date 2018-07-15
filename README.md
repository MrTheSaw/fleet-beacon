# fleet-beacon
**Software to flash Dotstar/Lumenati array with an Arduino-like controller.**
*Version: Ahahahaha, sure, ah, let's call it version .9 BETA*

The project started as a learning experience and to replace a quickly made, python/RaspberryPi based flashy thing with a cheaper device, freeing up the RPi for duties more appropriate to a full fledged general computing device.

This code is aimed at the Adafruit® Pro Trinket 5V, but ought to be portable to a number of Arduino® devices. I have actually run versions of it on a plain Trinket, and an ItsyBitsy 5V.

Basically, when this runs, it tells a DotStar array of some kind to do a dance. It's aimed at a Lumenati 8 Pack from SparkFun®, and a little bit hardcoded that way at the moment. It'd be nice to generalize it, when I have time, but at the moment, this is being shipped, and I would like to look at other projects.

Key hardware notes: The ItsyBitsy and Pro Trinket don't supply enough current for max draw on even a small array of these LEDs, so a more direct connect to power is required. I used a Power Boost 500, AdaFruit says you could prolly source a full amp out of it for a time, So you might be fine with 18-20 LEDs, especially if you aren't using it like a flashlight.

I chose the Lumenati due to the form factor, nothing more. I wanted the DotStar speed because the device might be video'ed and I wanted to try and avoid screen flashing. We'll see if it helps. That said, this could probably be reconfigured to use a NeoPixel compatible LED as well pretty easily.

The "dance" is controlled by a playlist which details what function to run, and how many times. This makes to creation and timing of dances a lot easier, and more like an animation.
