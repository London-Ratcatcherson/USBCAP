#include <stdlib.h>
#include <stdio.h>
#include "usbcap.h"
#include "resource.h"
#include <vfw.h>

HWND        hwnd;
HWND        hwndC;
HINSTANCE   hInst;

LRESULT CALLBACK WndProc (HWND, UINT, WPARAM, LPARAM) ;

char szAppName[] = "usbcap" ;
char szCaptureName[256];
char szDIBName[256];

/*
 * ResizeCaptureWindow - Resize the capture Window ....
 * This function resizes the capture window based upon the Video Capture
 * parameters found in the capStatus structure returned from Win32
 */
void ResizeCaptureWindow(HWND hwndC)
{
    CAPSTATUS    capStatus;

    // Get the capture window attributes .. width and height
    capGetStatus(hwndC, &capStatus, sizeof(capStatus));

    // Resize the capture window to the capture sizes
    SetWindowPos(hwndC, NULL, 0, 0,
        capStatus.uiImageWidth,    // Image width from camera
        capStatus.uiImageHeight,   // Image height from camera
        SWP_DRAWFRAME | SWP_NOMOVE | SWP_NOZORDER);
    return;
}

/*
 * ResizeMainWindow - Resize the main window and the capture Window ....
 * This function resizes the capture window based upon the Video Capture
 * parameters found in the capStatus structure returned from Win32
 */
void ResizeMainWindow(HWND hwnd, HWND hwndC)
{
    CAPSTATUS    capStatus;

    // Get the capture window attributes .. width and height
    capGetStatus(hwndC, &capStatus, sizeof(capStatus));

    // Resize the capture window to the capture sizes
    SetWindowPos(hwndC, NULL, 0, 0,
        capStatus.uiImageWidth,    // Image width from camera
        capStatus.uiImageHeight,   // Image height from camera
        SWP_DRAWFRAME | SWP_NOMOVE | SWP_NOZORDER);

    // Resize the main window to the capture sizes
    SetWindowPos(hwnd, NULL, 0, 0,
        capStatus.uiImageWidth+8,     // Image width from camera
        capStatus.uiImageHeight+46,   // Image height from camera
        SWP_DRAWFRAME | SWP_NOMOVE | SWP_NOZORDER);

    return;
}


LRESULT CALLBACK VideoStreamCallback(HWND hwndC, LPVIDEOHDR lpVHdr)
{
    printf("Video callback\n");
    //
    // Process Video Callbacks Here!!
    //
    return(0);
}

LRESULT CALLBACK WndProc (HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    MENUITEMINFO mii;

    switch (iMsg)
    {
        case WM_CREATE:
        {
            CAPTUREPARMS    captureParms;
            CAPDRIVERCAPS   caps;
            /*
             * Create the capture window
             */
            hwndC = capCreateCaptureWindow("Capture Window",
                WS_VISIBLE | WS_CHILD,
                0,0,
                0,0,
                hwnd, 0);

			if (hwndC == NULL)
			{
				MessageBox(hwnd, "You're DOOMED!  Can't create capture window!", "Capture Device Error", MB_OK);
				PostQuitMessage (0) ;            
                return 0 ;
			}

            // Connect the capture window to the driver
            if (!capDriverConnect(hwndC, 0))
			{
				//MessageBox(hwnd, "You're DOOMED!  Can't connect to driver!", "Capture Device Error", MB_OK);
				//PostQuitMessage (0) ;            
                //return 0 ;
			}

            // Get the capabilities of the capture driver
            capDriverGetCaps(hwndC, &caps, sizeof(caps));

            // If the capture driver does not support a dialog, grey it out
            //  In the menu bar.

            if (!caps.fHasDlgVideoSource)
            {
                EnableMenuItem(GetMenu(hwnd),ID_DISPLAY_DLG, MF_GRAYED);
            }
            if (!caps.fHasDlgVideoFormat)
            {
                EnableMenuItem(GetMenu(hwnd),ID_FORMAT_DLG, MF_GRAYED);
            }
            if (!caps.fHasDlgVideoDisplay)
            {
                EnableMenuItem(GetMenu(hwnd),ID_DISPLAY_DLG, MF_GRAYED);
            }

            mii.cbSize = sizeof(MENUITEMINFO);
            mii.fMask  = MIIM_STATE;

            mii.fState = MFS_GRAYED;
            SetMenuItemInfo(GetMenu(hwnd), ID_SAVEAS, FALSE, &mii);
            mii.fState = MFS_GRAYED;
            SetMenuItemInfo(GetMenu(hwnd), ID_SAVEDIB, FALSE, &mii);

            strcpy(szCaptureName, "C:\\USBCAP.AVI");
            strcpy(szDIBName, "C:\\USBCAP.DIB");

            // Set the video stream callback function
            capSetCallbackOnVideoStream(hwndC, VideoStreamCallback);

            if (MF_CHECKED & GetMenuState(GetMenu(hwnd), ID_PREVIEW, 0)) {
              // Start previewing the image from the camera
              capPreview(hwndC, TRUE);
              // Set the preview rate in milliseconds
              capPreviewRate(hwndC, 25);

            } else capPreview(hwndC, FALSE);

            // Resize the main window and the capture window to show the whole image
            ResizeMainWindow(hwnd, hwndC);

            return(0);
        }

    /*
     * A command has been received
     */
        case WM_COMMAND:
        {
            switch(LOWORD(wParam))
            {
    /*
     * If Exit is selected from the menu, end the program.
     */
                case ID_EXIT:
                    DestroyWindow(hwnd);
                    break;
    /*
     * If Start is selected from the menu, start Streaming capture.
     * The streaming capture is terminated when the Escape key is pressed
     */
                case ID_START:
                    capCaptureSequence(hwndC);
                    mii.cbSize = sizeof(MENUITEMINFO);
                    mii.fMask = MIIM_STATE;

                    mii.fState = MFS_ENABLED;
                    SetMenuItemInfo(GetMenu(hwnd), ID_SAVEAS, FALSE, &mii);                          
                    mii.fState = MFS_GRAYED;
                    SetMenuItemInfo(GetMenu(hwnd), ID_SAVEDIB, FALSE, &mii);
                    break;

    /*
     * Grab a single frame and ungrey/grey the appropriate menu options.
     */
                case ID_SINGLE_FRAME:
                    capGrabFrame(hwndC);
                    CheckMenuItem(GetMenu(hwnd), ID_PREVIEW, MF_UNCHECKED);
                    capPreview(hwndC, FALSE);
                    mii.cbSize = sizeof(MENUITEMINFO);
                    mii.fMask = MIIM_STATE;

                    mii.fState = MFS_GRAYED;
                    SetMenuItemInfo(GetMenu(hwnd), ID_SAVEAS, FALSE, &mii);
                    mii.fState = MFS_ENABLED;
                    SetMenuItemInfo(GetMenu(hwnd), ID_SAVEDIB, FALSE, &mii);
                    break;

    /*
     *  Do the SaveAs function.
     */
               case ID_SAVEAS:
                    capFileSaveAs(hwndC, szCaptureName);
                    break;

    /*
     *  Do the SaveDIB function.
     */
               case ID_SAVEDIB:
                    capFileSaveDIB(hwndC, szDIBName);
                    break;

    /*
     * Display the Video Format dialog when "Format" is selected from the
     * menu bar.
     */
                case ID_FORMAT_DLG:
                    capDlgVideoFormat(hwndC);
                    ResizeCaptureWindow(hwndC);
                    break;
    /*
     * Display the Video Display dialog when "Display" is selected from
     * the menu bar.
     */
                case ID_DISPLAY_DLG:
                    capDlgVideoDisplay(hwndC);
                    break;
    /*
     * Display the Compression dialog when "Compression" is selected from
     * the menu bar.
     */
                case ID_COMPRESSION_DLG:
                    capDlgVideoCompression(hwndC);
                    break;

   /*
    * If the Preview menu item is checked, the turn on the Preview
    */
                case ID_PREVIEW:
                    if (MF_CHECKED & GetMenuState(GetMenu(hwnd), ID_PREVIEW, 0)) {
                      CheckMenuItem(GetMenu(hwnd), ID_PREVIEW, MF_UNCHECKED);
                      capPreview(hwndC, FALSE);
                    } else {
                           CheckMenuItem(GetMenu(hwnd), ID_PREVIEW, MF_CHECKED);
                           capPreview(hwndC, TRUE);
                           capPreviewRate(hwndC, 25);
                    }
                    break;

    /*
     * Display the Video Source dialog when "Source" is selected from the
     * menu bar.
     */
                case ID_SOURCE_DLG:
                    capDlgVideoSource(hwndC);
                    break;
            }
            return(0);
        }

        case WM_PAINT:
        {
            HDC           hDC ;
            PAINTSTRUCT   ps ;

            hDC = BeginPaint(hwnd, &ps) ;

            // Included in case the background is not a pure color
            SetBkMode(hDC, TRANSPARENT) ;

            EndPaint(hwnd, &ps) ;
            return(0);
        }

        case WM_CLOSE:
            /*
             * Disable the video stream callback
             */
            capSetCallbackOnVideoStream(hwndC, NULL);
            break;

        case WM_DESTROY:
            PostQuitMessage (0) ;            
            return 0 ;

    }
    return DefWindowProc (hwnd, iMsg, wParam, lParam) ;
}

WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance,
                LPSTR lpszCmdParam, int iCmdShow)
{
    MSG        msg ;
    WNDCLASSEX wndclass ;

    hInst = hInstance;
    wndclass.cbSize        = sizeof (wndclass) ;
    wndclass.style         = CS_HREDRAW | CS_VREDRAW ;
    wndclass.lpfnWndProc   = WndProc ;
    wndclass.cbClsExtra    = 0 ;
    wndclass.cbWndExtra    = 0 ;
    wndclass.hInstance     = hInstance ;
    wndclass.hIcon         = LoadIcon (NULL, IDI_APPLICATION) ;
    wndclass.hCursor       = LoadCursor (NULL, IDC_ARROW) ;
    wndclass.hbrBackground = (HBRUSH) GetStockObject (WHITE_BRUSH) ;
    wndclass.lpszMenuName  = "menu";
    wndclass.lpszClassName = szAppName ;
    wndclass.hIconSm       = LoadIcon (NULL, IDI_APPLICATION) ;

    RegisterClassEx (&wndclass) ;

    hwnd = CreateWindow (szAppName, "USBCap",
                          WS_OVERLAPPEDWINDOW,
                          CW_USEDEFAULT, CW_USEDEFAULT,
                          320, 320,
                          NULL, NULL, hInstance, NULL) ;

    ShowWindow (hwnd, iCmdShow) ;
    UpdateWindow (hwnd) ;

    while (GetMessage (&msg, NULL, 0, 0))
    {
      TranslateMessage (&msg) ;
      DispatchMessage (&msg) ;
   }
   return msg.wParam ;
}

