# Square Line Studio - GUI Development
This markdown file gives instructions on how to setup SquareLine Studio for developing the GUI of the dashboard.

### Install Square Line Studio
This project used SquareLine Studio `1.4.1`. The installers can be found [here](https://squareline.io/downloads). Note projects can be upgraded to use newer versions of SquareLine Studio but cannot be downgraded.

A free account can be created with a personal licence. See [here](https://squareline.io/pricing/licenses) for more information.

## Getting Started
Open SquareLine studio and import the project .spj file. This will load up the work space.

From `File > Project Settings` scroll down to file export and set up the File Export as shown in below in the 'File Export' section.

When you want to use a GUI you have created in SquareLine Studio, click on `Export > Export UI Files`. Copy contents of `Dashboard/Dashboard-GUI/UI/` to `Dashboard_Programs/Dashboard_program`. Once that is done the code is ready to compile to use the new GUI.

## Project Settings

Note that Properties and Board properties will already be setup and are shown below as reference. The import section below is 'File Export'.

### Properties
Resolution: `480 x 320`\
Depth: `16 bit`\
Rotation: `0 degree`\
Offset X: `0`\
Offset Y: `0`\
Shape: `Rectange`

### Board Properties
Board Group: `Arduino`\
Board: `Arduino with TFT_eSPI`\
Version `v1.1.0`\
LVGL: `8.3.6`

### File Export
Project Export Root - Set this to the SquareLine Studio project directory. `~/<Path to Repository>/Dashboard/GUI`

UI Files Export Path - This can be any location of your choosing but is recommneded to be within the main dashboard directory like so. `~/<Path to Repository>/Dashboard/GUI/UI`

Multilanguage Support: `Disable`\
Call functions export file: `.c`\
Impage export mode: `Source code`\
Force export all images: `Checked`\
Flat export: `Checked`

All other fields in this section are left empty

### Project Description
QMFS Dashboard for 3.5" IPS TFT