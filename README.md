# First-ring blocker for Nomorobo

This tool blocks the first ring of all incoming calls, avoiding the annoying one-ring-and-silence from calls the great Nomorobo service blocks. This is at the cost of real callers having to wait one extra ring before we pick up.  Oh Well.

## Description

The real documentation of this project is here: [http://jimlaurwilliams.org/wordpress/?p=6738](http://jimlaurwilliams.org/wordpress/?p=6738) 

Its ATtiny85 basically controls a normally closed relay in series with the incoming phone line (here a VOIP ATA), opening the contacts a few milliseconds into the first ring, before phones can ring.  It reconnects quickly after the first ring, in hopes of not screwing with the Caller ID sent between rings.

This was built and tuned for my situation, with parts from my junkbox, and probably isn't a drop-in for anybody else.  OK, maybe if they have a Arris TG1682G modem providing their VOIP phone connections.  It was posted here at the request of someone who saw the post in my Project Notes.

The Eagle (7.7.0) files for the PCB I made up for mine are included here.  Unless your junkbox contains the same parts as mine, the board layout probably isn't useful.

Code for the main processor, an ATtiny85 on the PCB, developed with Arduino IDE 1.8.x, is  in **Nomorobo85Msg.ino**.  A Nano displays stats (rings, blocks) on a 1620 LCD.  Serial communication from the main PCB (via SoftwareSerial) tells the Nano what to show.  The Nano's code is **LcdHostNanoMsg1.ino**.  There are various comments and historical leftovers in both those files.

## Author

Jim Williams (a member of [Workshop88](http://workshop88.com))


## Acknowledgments

Inspired as an improvement on the very useful [Nomorobo](https://www.nomorobo.com/) service.

Special thanks to Aaron Foss for the cool Nomorobo T shirt!
