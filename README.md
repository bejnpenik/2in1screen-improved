# 2in1screen-improved
There exists some screenrotator but most depend on xrandr and xinput xorg apps. Wanted to learn more about randr extension and xinput extension and it bothered me that xrandr disables output while rotating...

# Installation

## Download

git clone https://github.com/bejnpenik/2in1screen-improved.git

cd 2in1screen-improved

## Compile

gcc -O2 -o sensors sensors.c -lm

gcc -O2 -o 2in1screen 2in1screen.c xcb_util_functions.c -lxcb -lxcb-randr -lxcb-xinput

CLIENT:

gcc -02 -o client client.c 

## Run

./2in1screen

## Options 

Rotate only touchscreen input matrix (useful if you wanna use external monitor and use laptop like wacom tablet )

--rotate-only-touchinput 

Dont have touchscreen input but still can rotate (Dunno if this even exists ... and even if it exists it is most uselless thing i ve heard of )

--rotate-only-output

Some bars like lemonbar or polybar dont repaint themselves while screen sizes change. It has to be done manually. Additionaly there needs to be way to lock rotation and easiest thing would be to communicate with bar widget (in my case lemonbar) so...
 
 --bar-path-command COMMAND_PATH
 
 Added a socket at /tmp/2in1screen.socket for better IPC or you can specify a path with arg:
 --socket-path SOCKET_PATH
 
 Additionally there is a little client which accepts following commands:
 
  rotate_left
  
  rotate_right
  
  rotate-normal
  
  rotate-inverted
  
  lock_rotation
  
  unlock_rotation
  
  toggle_lock
 
 ## Dependencies
  - xcb
  
  - xcb-randr
  
  - xcb-xinput
  
Most of you have this already

## Licence

No licence. Do as you wish.
  


