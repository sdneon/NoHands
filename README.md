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
    * Pink: disconnected.
  2. battery level:
    * Red: charging
    * Black: draining.
    * spoke rim thickness of 1 to 5 for 5 battery levels of <20% to 100%.
4. Random image popup at least once every hour.

### Screenshots
![screenshot 1](https://raw.githubusercontent.com/sdneon/NoHands/master/store/pebble-screenshot-1-AM.png "Watch face: AM, bluetooth connected, battery not charging")
Watch face: AM, bluetooth connected, battery not charging

![screenshot 2](https://raw.githubusercontent.com/sdneon/NoHands/master/store/pebble-screenshot-2-AM,DC,charging.png "Watch face: AM, bluetooth disconnected, battery charging")
Watch face: AM, bluetooth disconnected, battery charging

![screenshot 3](https://raw.githubusercontent.com/sdneon/NoHands/master/store/pebble-screenshot-3-PM,low-batt.png "Watch face: PM, bluetooth connected, battery low & not charging")
Watch face: PM, bluetooth connected, battery low & not charging

## Changelog
* v1.3
  * Hour is outlined so that it will be easier to see at all times.
  * Random (once every hour, and upon startup) surprise image popup for fun.
  * Changed to Pink Bluetooth indicator for disconnection.
* v1.2
  * Colour scheme changes.
    * Removed pairs of simliar colours.
    * Date is now always drawn in contrasting colour from background.
* v1.1
  * Initial release.
