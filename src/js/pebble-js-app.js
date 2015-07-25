//JSLint static code analysis options
/*jslint browser:true, unparam:true, sloppy:true, plusplus:true, indent:4, white:true */
/*global Pebble, console*/

var VERSION = 15,   //i.e. v1.4; for sending to config page
    //Defaults:
    DEF_VIBES = 0X00A14,
    watchConfig = {
        vibes: DEF_VIBES
    },
    //masks for vibes:
    MASKV_BTDC = 0x20000,
    MASKV_HOURLY = 0x10000,
    MASKV_FROM = 0xFF00,
    MASKV_TO = 0x00FF,
    vibes = 0,
    KEY_WEATHER = 'weather',
    UNIT_TEMPERATURE = {
        c: "\u00B0C",
        f: "\u00B0F"
    },
    timer;

var CLEAR_DAY = 0;
var CLEAR_NIGHT = 1;
var WINDY = 2;
var COLD = 3;
var PARTLY_CLOUDY_DAY = 4;
var PARTLY_CLOUDY_NIGHT = 5;
var HAZE = 6;
var CLOUD = 7;
var RAIN = 8;
var SNOW = 9;
var HAIL = 10;
var CLOUDY = 11;
var STORM = 12;
var NA = 13;

var imageId = {
  0 : STORM, //tornado
  1 : STORM, //tropical storm
  2 : STORM, //hurricane
  3 : STORM, //severe thunderstorms
  4 : STORM, //thunderstorms
  5 : HAIL, //mixed rain and snow
  6 : HAIL, //mixed rain and sleet
  7 : HAIL, //mixed snow and sleet
  8 : HAIL, //freezing drizzle
  9 : RAIN, //drizzle
  10 : HAIL, //freezing rain
  11 : RAIN, //showers
  12 : RAIN, //showers
  13 : SNOW, //snow flurries
  14 : SNOW, //light snow showers
  15 : SNOW, //blowing snow
  16 : SNOW, //snow
  17 : HAIL, //hail
  18 : HAIL, //sleet
  19 : HAZE, //dust
  20 : HAZE, //foggy
  21 : HAZE, //haze
  22 : HAZE, //smoky
  23 : WINDY, //blustery
  24 : WINDY, //windy
  25 : COLD, //cold
  26 : CLOUDY, //cloudy
  27 : CLOUDY, //mostly cloudy (night)
  28 : CLOUDY, //mostly cloudy (day)
  29 : PARTLY_CLOUDY_NIGHT, //partly cloudy (night)
  30 : PARTLY_CLOUDY_DAY, //partly cloudy (day)
  31 : CLEAR_NIGHT, //clear (night)
  32 : CLEAR_DAY, //sunny
  33 : CLEAR_NIGHT, //fair (night)
  34 : CLEAR_DAY, //fair (day)
  35 : HAIL, //mixed rain and hail
  36 : CLEAR_DAY, //hot
  37 : STORM, //isolated thunderstorms
  38 : STORM, //scattered thunderstorms
  39 : STORM, //scattered thunderstorms
  40 : STORM, //scattered showers
  41 : SNOW, //heavy snow
  42 : SNOW, //scattered snow showers
  43 : SNOW, //heavy snow
  44 : CLOUD, //partly cloudy
  45 : STORM, //thundershowers
  46 : SNOW, //snow showers
  47 : STORM, //isolated thundershowers
  3200 : NA //not available
};

//
// Weather stuff
//

function getWeatherFromWoeid(woeid) {
    var query = encodeURI("select item.condition from weather.forecast where woeid = " + woeid +
            " and u = \"" + watchConfig.units + "\""),
        url = "http://query.yahooapis.com/v1/public/yql?format=json&q=" + query,
        req = new XMLHttpRequest();

    req.open('GET', url, true);
    req.onload = function(e) {
        if (req.readyState === 4) {
            if (req.status === 200) {
                var response = JSON.parse(req.responseText),
                    condition, temperature, icon;
                if (response) {
                    condition = response.query.results.channel.item.condition;
                    temperature = condition.temp + UNIT_TEMPERATURE[watchConfig.units];
                    icon = imageId[condition.code];
                     console.log("temp " + temperature);
                     console.log("icon " + icon);
                     console.log("condition " + condition.text);
                    Pebble.sendAppMessage({
                        weather: watchConfig.weather? 1: 0,
                        icon : icon,
                        temperature : temperature
                    });
                }
            } else {
                console.log("Error");
            }
        }
    };
    req.send(null);
}

function getWeatherFromLatLong(latitude, longitude)
{
    var query = encodeURI("select woeid from geo.placefinder where text=\""+latitude+","+longitude + "\" and gflags=\"R\""),
        url = "http://query.yahooapis.com/v1/public/yql?q=" + query + "&format=json",
        req = new XMLHttpRequest();
    req.open('GET', url, true);
    req.onload = function(e) {
        if (req.readyState === 4)
        {
            if (req.status === 200)
            {
                var response = JSON.parse(req.responseText),
                    woeid;
                if (response)
                {
                    woeid = response.query.results.Result.woeid;
                    getWeatherFromWoeid(woeid);
                }
            }
            else
            {
                console.log("Error");
            }
        }
    };
    req.send(null);
}

function getWeatherFromLocation(location_name)
{
    var query = encodeURI("select woeid from geo.places(1) where text=\"" + location_name + "\""),
        url = "http://query.yahooapis.com/v1/public/yql?q=" + query + "&format=json",
        req = new XMLHttpRequest();
    req.open('GET', url, true);
    req.onload = function(e) {
        if (req.readyState === 4)
        {
            if (req.status === 200)
            {
                 console.log(req.responseText);
                var response = JSON.parse(req.responseText),
                    woeid;
                if (response)
                {
                    woeid = response.query.results.place.woeid;
                    getWeatherFromWoeid(woeid);
                }
            }
            else
            {
                console.log("Error");
            }
        }
    };
    req.send(null);
}

var locationOptions = { "timeout": 15000, "maximumAge": 60000 };

function locationSuccess(pos)
{
    var coordinates = pos.coords;
    getWeatherFromLatLong(coordinates.latitude, coordinates.longitude);
}

function locationError(err)
{
    console.warn('location error (' + err.code + '): ' + err.message);
    Pebble.sendAppMessage({
        weather: watchConfig.weather? 1: 0,
        icon: 11,
        temperature: ""
    });
}

function updateWeather()
{
    if (!watchConfig.weather)
    {
        return;
    }
    if (watchConfig.gps)
    {
        window.navigator.geolocation.getCurrentPosition(locationSuccess,
            locationError,
            locationOptions);
    }
    else
    {
        getWeatherFromLocation(watchConfig.location);
    }
}

//
// Config stuff
//

//Load saved config from localStorage
function loadConfig()
{
    var data = localStorage.getItem(KEY_WEATHER);
    if (data !== undefined)
    {
        data = JSON.parse(data);
    }
    watchConfig = data || {
        weather: 0,
        gps: true,
        location: '',
        units: 'c',
        interval: 1800000 // 30 minutes
    };
    data = parseInt(localStorage.getItem(vibes), 10);
    if (isNaN(data))
    {
        data = DEF_VIBES;
    }
    watchConfig.vibes = data;
}

//Save config to localStorage
function saveConfig()
{
    localStorage.setItem(vibes, watchConfig.vibes);
    localStorage.setItem(KEY_WEATHER, JSON.stringify(watchConfig));
}

function sendOptions(options)
{
    Pebble.sendAppMessage(options,
        function(e) {
            console.log('Successfully delivered message');
        },
        function(e) {
            console.log('Unable to deliver message');
        }
    );
}

Pebble.addEventListener('webviewclosed',
    function(e) {
        //console.log('Configuration window returned: ' + e.response);
        if (!e.response)
        {
            return;
        }
        var options = JSON.parse(e.response);
        if (!options)
        {
            return;
        }
        watchConfig.weather = options.weather;
        watchConfig.gps = options.gps;
        watchConfig.location = options.location;
        watchConfig.units = options.units;
        watchConfig.vibes = parseInt(options.vibes, 10);
        watchConfig.interval = options.interval;

        sendOptions(watchConfig);
        saveConfig();

        setTimeout(function() {
            updateWeather();
        }, 500);

        if (!watchConfig.weather)
        {
            if (timer !== undefined)
            {
                clearInterval(timer);
                timer = undefined;
            }
        }
        else if (!timer)
        {
            timer = setInterval(function() {
                //console.log("timer fired");
                updateWeather();
            }, watchConfig.interval);
        }
    }
);


Pebble.addEventListener('showConfiguration',
    function(e) {
        try {
            var url = 'http://yunharla.altervista.org/pebble/config-nohands.html?ver=' + VERSION + '&vibes=0x';
            //Send/show current config in config page:
            url += watchConfig.vibes.toString(16)
                + '&weather=' + !!watchConfig.weather
                + '&gps=' + !!watchConfig.gps
                + '&location=' + watchConfig.location
                + '&units=' + watchConfig.units
                + '&interval=' + watchConfig.interval;

            // Show config page
            Pebble.openURL(url);
        }
        catch (ex)
        {
            console.log('ERR: showConfiguration exception');
        }
    }
);

Pebble.addEventListener('ready',
    function(e) {
        console.log('ready');
        loadConfig();
        sendOptions(watchConfig);
        if (watchConfig.weather)
        {
            updateWeather();
            timer = setInterval(function() {
                //console.log("timer fired");
                updateWeather();
            }, watchConfig.interval);
        }
    });
