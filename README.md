# GGScore
Application to create power rankings via the Glicko-2 algorithm.

The basic idea of this application is to create Power Rankings.
Originally for competitive Super Smash Bros. for the Nintendo Gamecube, but you can technically create PRs for every competitive discipline.
The ratings are created via the Glicko-2 algorithm.  
A ready-to-go build can be found in [this dropbox][Download link for build].

## Basic usage/features

The general idea to use this tool is by importing tournaments from [challonge.com](https://www.challonge.com/). For this you will need a developer API key from your challonge-account.
Then the players' names/aliases from the tournament's sets have to be associated with players. You can either associate an alias with an existing player or create a new one (when importing the first tournament you will have to create all new players of course). The results consisting of the winner, loser and date of the match will be added to the table in the "Match Report"-tab and can be manually removed (or added), if necessarry. 
Once you imported your tourneys you have to enter a rating period. The algorithm will only take results into account that happened within a rating period. It is recommended to let a player have at least 10-15 games in a rating period to make his rating accurate (especially for the first rating period).
If the "Match Report" gets too clustered you can use the "Finalize Ratings"-option in the "Rating Periods"-tab (where the actual PR is shown as well) to clear the tables displaying the results and rating periods. This also means that you will not be able to remove/add results from the rating periods existing before finalizing though (thus finalizing the ratings up to that point). However, you can add/remove and import results as before from then on.

Keep in mind that only results within a rating period will be used to calculate a rating.
 Also all results within a rating period are seen as happening at once (by the definition of the Glicko-2 algorithm). That means if you create all players with the same starting values (intended use) there will only be matches of people with exactly the same values. So, if there's a PR like this with only one rating period currently and all players started with 1500 rating points and you enter a result of a (currently) 1000-rated player winning against a (currently) 2000-rated player, the whole calculation for that period will be repeated and that result is factored in as a 1500-rated player winning against a 1500-rated player. 
 So the first period is more about getting a general idea of the rating values and upsets will have less influence. 
 This is probably a pretty common behaviour for a rating algorithm though. To avoid this you would have to supply accurate starting values, but the average user probably shouldn't (I wouldn't).
 
 
## About the implementation
 
 This application consists of two modules: The Glicko-2 API, which supplies the Glicko-2 algorithm and the GUI supplying the user with means to use the API and visualize the results of the calculations. It also associates the IDs of the Glicko-Players with their aliases.
 
 ### Glicko-2 API
 
 The Glicko-2 API consists of various classes to set up the needed conditions to apply the algorithm and of course the algorithm itself:
 
 - Glicko-2 -
This is the main class containing the actual algorithm and using the other classes to build the "Glicko world"
 - World -
 This class is mainly used to hold and manage the player base. It also saves the system constant "Tau".
 - Player -
 Represents a player consisting of an ID and the rating values being the actual rating, the deviation and the volatility.
 This class is mainly used by the Glicko-2 API to create players in the world, but it also supplies the user with getters to read the ID and rating values.
 - Result -
 Represents a result, this class holds the IDs for player 1, player 2 and the winner's ID. If you want to report a tie the winner ID must be something different than the players' IDs (-1 is recommended though).
 Results do not get created internally neither are they stored. This class is meant to be used by the user to construct and bundle results (per rating period) and then give them to the algorithm.
 
 ### GUI
 
 The GUI implements means for the user to use the API and display the results of the algorithm. It also manages the aliases of the players, associating them with the ID of their Glicko-counterpart. 
 Besides that it creates all results (using the Glicko-Result class) and saves them together with the date they were entered to have occured.  
The Glicko-2 API just applies the algorithm to existing players with whatever results are passed to it. So the rating periods are implemented here as well to bundle results and give them to the algorithm.
 
 To be continued...
 
 ### Date format
 
 If you build the application as is from this repo the date format will be dd.mm.yyyy, which can be changed in the GlobalVars.h file. 
 The "defaultFormatString" can be changed using the specifiers shown [here](http://www.cplusplus.com/reference/ctime/strftime/). 
 The standard US date format will be "%m/%d/%Y" and a build can be found in the [dropbox][Download link for build].  
 Changing the date format can be done without worrying about the save-files (mainly results.json, being the only one saving dates). The dates are always saved in ISO 8601 format and only displayed as described by the defaultFormatString-value.
 
 ## How to build
 
 This application uses a few APIs which will have to be built and linked before you will be able to build this project.
 The needed APIs are [wxWidgets](https://wxwidgets.org/), [jsoncpp](https://github.com/open-source-parsers/jsoncpp) and [libcurl](https://curl.haxx.se/libcurl/) (7.62.0 used, but others should work of course, but you will have to change the path in the supplied project file, if you want to use it).
 The supplied Visual Studio project files require the following folder structure to successfully build the application:
 
 ```
 └── ┐
     ├── GGScore
     │   └── .git
     │   └── PR Tool 
     ├── wxWidget
     │   ├── lib
     │   └── include
     ├── jsoncpp-master
     │   └── dist
     └── curl-7.62.0
         ├── include
         └── lib
```
 These APIs are statically linked, only the Glicko-2 API is linked dynamically (again, only speaking for the supplied project-file).

### Known issue(s)


[Download link for build]: https://www.dropbox.com/sh/0f5kgb1ol10l076/AADQnKghLpe9jwq4Mr67W5Cwa?dl=0