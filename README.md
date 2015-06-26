No Hands
========

Simple analogue watch with coloured sectors and hour hint for Pebble Time. (Colour, Analogue).

Inspired by [Chromatick](http://chromatickface.tumblr.com/concept).

## Display
1. Hour shown to easily tell time.
  * White: PM,
  * Black: AM.
2. Day of week & date (DDMM) in 2 rows.
3. Middle spoke indicator for bluetooth connectivity & battery level.
  1. bluetooth indicated by colour of centre:
    * White: connected,
    * Light gray: disconnected.
  2. battery level:
    * Red: charging
    * Black: draining.
    * spoke rim thickness of 1 to 5 for 5 battery levels of <20% to 100%.

### Screenshots
![screenshot 1](https://raw.githubusercontent.com/sdneon/NoHands/master/store/pebble-screenshot-1-AM.png "Watch face: AM, bluetooth connected, battery not charging")
Watch face: AM, bluetooth connected, battery not charging

![screenshot 2](https://raw.githubusercontent.com/sdneon/NoHands/master/store/pebble-screenshot-2-AM,DC,charging.png "Watch face: AM, bluetooth disconnected, battery charging")
Watch face: AM, bluetooth disconnected, battery charging

![screenshot 3](https://raw.githubusercontent.com/sdneon/NoHands/master/store/pebble-screenshot-3-PM,low-batt.png "Watch face: PM, bluetooth connected, battery low & not charging")
Watch face: PM, bluetooth connected, battery low & not charging

## Changelog
* v1.2
  * Colour scheme changes.
    * Removed pairs of simliar colours.
    * Date is now always drawn in contrasting colour from background.
* v1.1
  * Initial release.

## TODOs
* Colour scheme needs to be fixed.
  * Bluetooth indicator colour of white & light gray may be too hard to differentiate.
  * Hour display is sometimes hard to read owing to poor contrast. Perhaps add a shadow?
* Add random (say occuring at a random minute each hour) animated peek-through images in the other empty quadrant.
