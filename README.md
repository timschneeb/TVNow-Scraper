# TVNow/Watchbox Scraper

## Features

* Extract M3U hls stream playlist file from Movie ID

## Usage

Note: Only movies supported for now.

Extract the movie id from the url:

```
https://www.tvnow.de/filme/pokemon-14-weiss-victini-und-zekrom-17225
```

In this case `17225` is the movie id.

Now you can simply run the program with the id as the first argument and it will place the generated M3U file into the working directory (run it in the command line!):

```
./tvnow-scraper 17225

____________Detected Movie/Show____________
Title:	Pokémon 14: Weiß - Victini und Zekrom
ID:	17225
Movie?:	1
___________________________________________
Downloading 1920x1080
```

