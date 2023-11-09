#ifndef __OBJECT_DRAWING
#define __OBJECT_DRAWING

#include <string>
#include <sstream>
#include <iostream>

#include "TCanvas.h"
#include "TAxis.h"
#include "TGraph.h"
#include "TGraphErrors.h"
#include "TH1.h"
#include "TH2.h"
#include "TF1.h"
#include "TPaveStats.h"
#include "TPaletteAxis.h"

/**
 * @brief DrawTF1 draws a TF1 Object onto a canvas that can be provided or will be created
 * @param func          - pointer to the object instance to draw
 * @param c             - optionally a pointer to a canvas. If a nullptr is passed, a new canvas
 *                          will be created
 * @param title         - title for the plot
 * @param xtitle        - X axis title
 * @param ytitle        - Y axis title
 * @param drawoptions   - drawing options. Make sure to add a "same" here if it is supposed to be
 *                          plotted in an existing canvas and plot
 * @return              - a pointer to the canvas with the plot (it is the same as `c` if it was
 *                          not a nullptr)
 */
TCanvas* DrawTF1(TF1* func, TCanvas* c = nullptr, std::string title = "",
                 std::string xtitle = "", std::string ytitle = "",
                 std::string drawoptions = "");

/**
 * @brief DrawTGraph draws a TGraph Object onto a canvas that can be provided or will be created
 * @param gr            - pointer to the object instance to draw
 * @param c             - optionally a pointer to a canvas. If a nullptr is passed, a new canvas
 *                          will be created
 * @param title         - title for the plot
 * @param xtitle        - X axis title
 * @param ytitle        - Y axis title
 * @param drawoptions   - drawing options. Make sure to add a "same" here if it is supposed to be
 *                          plotted in an existing canvas and plot
 * @return              - a pointer to the canvas with the plot (it is the same as `c` if it was
 *                          not a nullptr)
 */
TCanvas* DrawTGraph(TGraph* gr, TCanvas* c = nullptr, std::string title = "",
                    std::string xtitle = "", std::string ytitle = "",
                    std::string drawoptions = "AP");

/**
 * @brief DrawTGraphErrors draws a TGraphErrors Object onto a canvas that can be provided or will
 *          be created
 * @param gr            - pointer to the object instance to draw
 * @param c             - optionally a pointer to a canvas. If a nullptr is passed, a new canvas
 *                          will be created
 * @param title         - title for the plot
 * @param xtitle        - X axis title
 * @param ytitle        - Y axis title
 * @param drawoptions   - drawing options. Make sure to add a "same" here if it is supposed to be
 *                          plotted in an existing canvas and plot
 * @return              - a pointer to the canvas with the plot (it is the same as `c` if it was
 *                          not a nullptr)
 */
TCanvas* DrawTGraphErrors(TGraphErrors* gr, TCanvas* c = nullptr, std::string title = "",
                          std::string xtitle = "", std::string ytitle = "",
                          std::string drawoptions = "APE");

/**
 * @brief DrawTH1 draws a TH1 on a canvas or creates a new canvas to draw on
 * @param hist          - the object instance of the histogram to draw
 * @param c             - optional pointer to an existing canvas to draw on. If a nullptr is
 *                          passed, a new canvas will be created before drawing
 * @param title         - title for the plot
 * @param xtitle        - X axis title
 * @param ytitle        - Y axis title
 * @param drawoptions   - drawing options for TH1::Draw(). Make sure to use the "same" option
 *                          if drawing into an existing plot (and `c` was not a nullptr)
 * @param stats         - display statistics box on true
 * @return              - a pointer to the canvas with the plot (it is the same as `c` if it was
 *                          not a nullptr)
 */
TCanvas* DrawTH1(TH1* hist, TCanvas* c = nullptr, std::string title = "",
                 std::string xtitle = "", std::string ytitle = "",
                 std::string drawoptions = "", bool stats = true);

/**
 * @brief DrawTH2 draws a TH2 on a canvas or creates a new canvas to draw on
 * @param hist          - the object instance of the histogram to draw
 * @param c             - optional pointer to an existing canvas to draw on. If a nullptr is
 *                          passed, a new canvas will be created before drawing
 * @param title         - title for the plot
 * @param xtitle        - X axis title
 * @param ytitle        - Y axis title
 * @param ztitle        - Z axis title
 * @param drawoptions   - drawing options for TH1::Draw(). Make sure to use the "same" option
 *                          if drawing into an existing plot (and `c` was not a nullptr)
 * @param stats         - display statistics box on true
 * @return              - a pointer to the canvas with the plot (it is the same as `c` if it was
 *                          not a nullptr)
 */
TCanvas* DrawTH2(TH2* hist, TCanvas* c = nullptr, std::string title = "",
                 std::string xtitle = "", std::string ytitle = "", std::string ztitle = "",
                 std::string drawoptions = "colz", bool stats = true);


/**
 * @brief MoveStatsBox changes the position of the stats box for the canvas
 * @param c             - the canvas for which the stats box is to be moved
 * @param x1ndc         - new x position start with x in [0,1], < 0 defaults to 0.62
 * @param y1ndc         - new y position start with y in [0,1], < 0 defaults to 0.15
 * @param x2ndc         - new x position end with x in [0,1], < 0 defaults to 0.82
 * @param y2ndc         - new y position end with y in [0,1], < 0 defaults to 0.39
 * @param name          - Name (alias title) for the stats box
 * @return              - true on success, false otherwise (e.g. stats box missing as turned off)
 */
bool MoveStatsBox(TCanvas* c, double x1ndc = -1, double y1ndc = -1,
                  double x2ndc = -1, double y2ndc = -1, std::string name = "");

/**
 * This is a list of warning levels that can be used in ROOT. The values do not match the internal
 *    ROOT implementation. There, the values have to be multiplied by 1000
 */
enum WarningLevels {
    WL_Print    = 0,
    WL_Info     = 1,
    WL_Warning  = 2,
    WL_Error    = 3,
    WL_Break    = 4,
    WL_SysError = 5,
    WL_Fatal    = 6
};

/// The following functions were created from information found at "https://root.cern.ch/root/roottalk/roottalk10/1008.html"

/**
 * receive back the waring level as `WarningLevels` value. Everything smaller than the value
 *   is ignored.
 */
int GetROOTWarningLevel();

/**
 * changes the warning level for the current session. The parameter is a value from `WarningLevels`
 *   everything with a value smaller than the passed one will not be output anymore.
 *
 * @param level         - new ignore level for output by ROOT.
 * @return              - the new ignore level that was set. If it does not match the value of 
 *                          `level`, the value was not accepted
 */
int SetROOTWarningLevel(int level);

/**
 * Saves the content of a canvas to a file optionally suppressing the painter information message
 *  from ROOT that a file has been created
 *
 * @param c             - pointer to the TCanvas object to save
 * @param filename      - filename to save the TCanvas object to
 * @param noinfo        - on true, suppress the saving info message by temporarily changing the 
 *                          warning level
 */
void SaveCanvas(TCanvas* c, std::string filename, bool noinfo = true);

#endif //__OBJECT_DRAWING
