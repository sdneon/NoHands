No Hands
========

Simple analogue watch with coloured sectors and hour hint for Pebble Time. (Colour, Analogue, Configurable). Now with optional vibrations & weather.

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
5. Optional weather info using Yahoo Weather data: weather condition and temperature.
 * Location (using GPS or predefined locaiton), update interval and temperature units are configurable.
 * Yahoo has 47 different weather conditions. Several similar ones with differing adjectives like- isolated, scattered.
   * Isolated is ~10-20% coverage (area affected), and depicted by weather icon in the far distance (so it's partly cropped/clipped).
   * Scattered is ~30-50% coverage, and depicted by weather icon in middle distance (less cropping than for _isolated_).

## Vibes
Optional vibrations for:
    * Bluetooth connection lost: fading vibe.
    * Hourly chirp. Default: Off, 10am to 8pm.

### Screenshots
![screenshot 1](https://raw.githubusercontent.com/sdneon/NoHands/master/store/pebble-screenshot-1-AM.png "Watch face: AM, bluetooth connected, battery not charging")
Watch face: AM, bluetooth connected, battery not charging

![screenshot 2](https://raw.githubusercontent.com/sdneon/NoHands/master/store/pebble-screenshot-2-AM,DC,charging.png "Watch face: AM, bluetooth disconnected, battery charging")
Watch face: AM, bluetooth disconnected, battery charging

![screenshot 3](https://raw.githubusercontent.com/sdneon/NoHands/master/store/pebble-screenshot-3-PM,low-batt.png "Watch face: PM, bluetooth connected, battery low & not charging")
Watch face: PM, bluetooth connected, battery low & not charging

## Changelog
* v1.5
  * Add optional weather: condition icon & temperature.
    * 47 icons for Yahoo Weather conditions. Extended many of them from [Timely watchface icons' ](https://github.com/cynorg/PebbleTimely) style.
* v1.4
  * Added optional vibes for:
    * Bluetooth connection lost: fading vibe.
    * Hourly chirp. Default: Off, 10am to 8pm.
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

### TODO
* Weather:
  * Move update code out of app message handler, so that handler will be free to handle next incoming message soon. (Currently, delaying JS weather update by 1 sec to avoid ERROR 64-APP_MSG_BUSY.)
  * Add black set of weather icons for light background?

## Credits
Thanks to sample weather codes & icons from tallerthenyou's [Simple Weather watchface](https://github.com/tallerthenyou/simplicity-with-day) and alexsum's [Timely watchface ](https://github.com/cynorg/PebbleTimely).