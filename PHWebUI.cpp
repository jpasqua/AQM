/*
 * PHWebUI:
 *    Implements a simple WebUI for PurpleHaze
 *                    
 * NOTES:
 *
 */


//--------------- Begin:  Includes ---------------------------------------------
//                                  Core Libraries
//                                  Third Party Libraries
#include <ArduinoLog.h>
//                                  Local Includes
#include "PurpleHaze.h"
#include "PHWebUI.h"
#include "src/utils/Output.h"
#include "src/clients/AQIMgr.h"
//--------------- End:    Includes ---------------------------------------------



// ----- BEGIN: WebUI namespacea
namespace PHWebUI {
  static const String   checkedOrNot[2] = {"", "checked='checked'"};

  // ----- BEGIN: PHWebUI::Internal
  namespace Internal {
    String Actions =
      "<a class='w3-bar-item w3-button' href='/ChartPage.html'>"
      "<i class='fa fa-bar-chart'></i> Charts</a>"
      "<a class='w3-bar-item w3-button' href='/displayPHConfig'>"
      "<i class='fa fa-cog'></i> Configure PH</a>";
    String DEV_ACTION =
      "<a class='w3-bar-item w3-button' href='/dev'>"
      "<i class='fa fa-gears'></i> Dev Settings</a>";

    constexpr uint32_t BusyColor = 0xff88ff;
    void showBusyStatus(bool busy) {
      if (busy) PH::busyIndicator->setColor(BusyColor);
      else PH::busyIndicator->off();
    }
  }
  // ----- END: PHWebUI::Internal


  // ----- BEGIN: PHWebUI::Endpoints
  namespace Pages {

    // Displays the home page which shows the last set of weather readings.
    //
    // Form:
    //    GET  /displayHomePage
    //
    void displayHomePage() {
      auto mapper =[](const String &key, String& val) -> void {
        const AQIReadings& aqiReadings = PH::aqiMgr.getLastReadings();
        if      (key == "LAT")      val = WebThing::settings.latAsString();
        else if (key == "LNG")      val = WebThing::settings.lngAsString();
        else if (key == "AQI")      val.concat(PH::aqiMgr.derivedAQI(aqiReadings.env.pm25));
        else if (key == "PM10STD")  val.concat(aqiReadings.standard.pm10);
        else if (key == "PM25STD")  val.concat(aqiReadings.standard.pm25);
        else if (key == "PM100STD") val.concat(aqiReadings.standard.pm100);
        else if (key == "PM10ENV")  val.concat(aqiReadings.env.pm10);
        else if (key == "PM25ENV")  val.concat(aqiReadings.env.pm25);
        else if (key == "PM100ENV") val.concat(aqiReadings.env.pm100);
        else if (key == "P03")      val.concat(aqiReadings.particles_03um);
        else if (key == "P05")      val.concat(aqiReadings.particles_05um);
        else if (key == "P10")      val.concat(aqiReadings.particles_10um);
        else if (key == "P25")      val.concat(aqiReadings.particles_25um);
        else if (key == "P50")      val.concat(aqiReadings.particles_50um);
        else if (key == "P100")     val.concat(aqiReadings.particles_100um);
        else if (key == "TMST")     val = Output::dateTime(aqiReadings.timestamp);
        else if (key == "MA10")     val.concat(round(PH::aqiMgr.pm25env_10min.getAverage()));
        else if (key == "MA30")     val.concat(round(PH::aqiMgr.pm25env_30min.getAverage()));
        else if (key == "MA1H")     val.concat(round(PH::aqiMgr.pm25env_1hr.getAverage()));
        else if (key == "MA6H")     val.concat(round(PH::aqiMgr.pm25env_6hr.getAverage()));
      };

      WebUI::wrapWebPage("/", "/HomePage.html", mapper);
    }

    void displayChartPage() {
      auto mapper =[](const String &key, String& val) -> void {
        if      (key == "PM10_CLR")  val = PH::settings.chartColors.pm10;
        else if (key == "PM25_CLR")  val = PH::settings.chartColors.pm25;
        else if (key == "PM100_CLR") val = PH::settings.chartColors.pm100;
        else if (key == "AQI_CLR")   val = PH::settings.chartColors.aqi;
      };

      WebUI::wrapWebPage("/displayChartPage", "/ChartPage.html", mapper);
    }

    // Displays a form allowing the user to update the PurpleHaze settings.
    //
    // Form:
    //    GET  /displayPHConfig
    //
    void displayPHConfig() {
      auto mapper =[](const String &key, String& val) -> void {
        if      (key == "DESC")       val = WebThing::encodeAttr(PH::settings.description);
        else if (key == "BLYNK_KEY")  val = PH::settings.blynkAPIKey;
        else if (key == "I_BRIGHT")   val.concat(PH::settings.iBright);
        else if (key == "PM10_CLR")   val = PH::settings.chartColors.pm10;
        else if (key == "PM25_CLR")   val = PH::settings.chartColors.pm25;
        else if (key == "PM100_CLR")  val = PH::settings.chartColors.pm100;
        else if (key == "AQI_CLR")    val = PH::settings.chartColors.aqi;
        else if (key == "TEMP_CORRECT") val.concat(PH::settings.bmeSettings.tempCorrection);
        else if (key == "HUMI_CORRECT") val.concat(PH::settings.bmeSettings.humiCorrection);
        else if (key == "TEMP_CLR")   val = PH::settings.bmeSettings.chartColors.temp;
        else if (key == "AVG_CLR")    val = PH::settings.bmeSettings.chartColors.avg;
      };

      WebUI::wrapWebPage("/displayPHConfig", "/ConfigForm.html", mapper);
    }
  }   // ----- END: PHWebUI::Pages


  namespace Endpoints {
    void getHistory() {
      auto action = []() {
        String rangeArg = WebUI::arg("range");
        AQIMgr::HistoryRange range;

        if (rangeArg.equalsIgnoreCase("hour")) range = AQIMgr::HistoryRange::Range_1Hour;
        else if (rangeArg.equalsIgnoreCase("day")) range = AQIMgr::HistoryRange::Range_1Day;
        else if (rangeArg.equalsIgnoreCase("week")) range = AQIMgr::HistoryRange::Range_1Week;
        else range = AQIMgr::HistoryRange::Range_Combined;

        auto provider = [range](Stream& s) -> void {
          PH::aqiMgr.emitHistoryAsJson(range, s);
        };

        WebUI::sendArbitraryContent("application/json", -1, provider);
      };

      WebUI::wrapWebAction("/getHistory", action, false);
    }

    void getAQI() {
      auto action = []() {
        String result;
        result.reserve(300);
        const AQIReadings& aqiReadings = PH::aqiMgr.getLastReadings();
        PH::aqiMgr.aqiAsJSON(
          PH::aqiMgr.derivedAQI(aqiReadings.env.pm25),
          aqiReadings.timestamp, result);
        WebUI::sendStringContent("application/json", result);
      };

      WebUI::wrapWebAction("/getHistory", action, false);
    }

    // Handler for the "/updatePHConfig" endpoint. This is invoked as the target
    // of the form presented by "/displayPHConfig". It updates the values of the
    // corresponding settings and writes the settings to EEPROM.
    //
    // TO DO: Call a function to let the main app know that settings may have changed
    //        so that it can take any appropriate actions
    //
    // Form:
    //    GET /updatePHConfig?description=DESC&iBright=INT&...
    //
    void updatePHConfig() {
      auto action = []() {
        PH::settings.description = WebUI::arg("description");
        PH::settings.blynkAPIKey = WebUI::arg("blynkAPIKey");
        PH::settings.iBright = (constrain(WebUI::arg("iBright").toInt(), 0, 100));
        PH::settings.chartColors.pm10 = WebUI::arg("pm10Color");
        PH::settings.chartColors.pm25 = WebUI::arg("pm25Color");
        PH::settings.chartColors.pm100 = WebUI::arg("pm100Color");
        PH::settings.chartColors.aqi = WebUI::arg("aqiColor");
        PH::settings.write();

        // The description MAY have changed. Update the title just in case
        PH::setIndicatorBrightness(PH::settings.iBright);
        WebUI::setTitle(PH::settings.description+" ("+WebThing::settings.hostname+")");
        WebUI::redirectHome();
      };

      WebUI::wrapWebAction("/updatePHConfig", action, false);
    }
  }   // ----- END: PHWebUI::Endpoints

  namespace Dev {
    void updateSettings() {
      auto action = []() {
        PH::settings.showDevMenu = WebUI::hasArg("showDevMenu");
        PH::settings.write();
        WebUI::redirectHome();
      };
      WebUI::wrapWebAction("/updateSettings", action, false);
    }

    void displayDevPage() {
      auto mapper =[](const String &key, String& val) -> void {
        if (key == "SHOW_DEV_MENU") val = checkedOrNot[PH::settings.showDevMenu];
      };

      WebUI::wrapWebPage("/displayDevPage", "/DevPage.html", mapper);
    }

    void reboot() {
      if (!WebUI::authenticationOK()) { return; }
      WebUI::redirectHome();
      ESP.restart();
    }

    void yieldSettings() {
      auto action = []() {
        DynamicJsonDocument *doc = (WebUI::hasArg("wt")) ? WebThing::settings.asJSON() :
                                                           PH::settings.asJSON();
        WebUI::sendJSONContent(doc);
        doc->clear();   // TO DO: Is this needed?
        delete doc;     // TO DO: Is this needed?
      };
      WebUI::wrapWebAction("/updateSettings", action, false);
    }

  }   // ----- END: PHWebUI::Dev


  void init() {
    WebUI::setTitle(PH::settings.description+" ("+WebThing::settings.hostname+")");
    String actions = Internal::Actions;
    if (PH::settings.showDevMenu) {
      actions += Internal::DEV_ACTION;
    }
    WebUI::addMenuItems(actions);

    WebUI::registerBusyCallback(Internal::showBusyStatus);

    WebUI::registerHandler("/", Pages::displayHomePage);
    WebUI::registerHandler("/ChartPage.html",     Pages::displayChartPage);
    WebUI::registerHandler("/displayPHConfig",    Pages::displayPHConfig);

    WebUI::registerHandler("/updatePHConfig",     Endpoints::updatePHConfig);
    WebUI::registerHandler("/getHistory",         Endpoints::getHistory);
    WebUI::registerHandler("/getAQI",             Endpoints::getAQI);

    WebUI::registerHandler("/dev",                Dev::displayDevPage);
    WebUI::registerHandler("/dev/reboot",         Dev::reboot);
    WebUI::registerHandler("/dev/settings",       Dev::yieldSettings);
    WebUI::registerHandler("/dev/updateSettings", Dev::updateSettings);
  }

}
// ----- END: PHWebUI
