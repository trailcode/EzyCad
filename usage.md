# EzyCad Usage Guide

## Table of Contents
1. [Introduction](#introduction)
2. [Getting Started](#getting-started)
3. [User Interface](#user-interface)
4. [File Operations](#file-operations)
5. [Modeling Tools](#modeling-tools)
6. [Keyboard Shortcuts](#keyboard-shortcuts)
7. [View Controls](#view-controls)
8. [Tips and Tricks](#tips-and-tricks)
9. [Support](#support)
10. [Tool Icons](#tool-icons)

## Introduction

EzyCad is a modern CAD (Computer-Aided Design) application designed for creating and manipulating 2D and 3D models. It leverages OpenGL, ImGui, and Open CASCADE Technology (OCCT) to provide a robust and interactive user interface for CAD operations.

## Getting Started

### System Requirements
- Windows, Linux, or macOS operating system
- OpenGL-compatible graphics card
- Minimum 4GB RAM recommended
- 500MB free disk space

### Installation
1. Download the latest release for your operating system
2. Extract the archive to your preferred location
3. Run the executable file

## User Interface

### Main Components
1. **Menu Bar**
   - File operations (New, Open, Save, Import)
   - Edit functions
   - View controls
   - Help and settings

2. **Toolbar**
   - Quick access to commonly used tools
   - Mode selection buttons
   - Operation tools

3. **Sketch List**
   - View and manage 2D sketches
   - Select and edit sketch elements
   - Toggle sketch visibility

4. **Shape List**
   - View and manage 3D shapes
   - Select and edit shape properties
   - Toggle shape visibility

5. **Options Panel**
   - Adjust tool parameters
   - Set operation properties
   - Configure view settings

6. **Log Window**
   - View operation history
   - Check for errors and warnings
   - Monitor system status

## File Operations

### Supported Formats
- Native format: `.ezy` files
- Import formats: STEP, IGES, STL
- Export formats: STEP, IGES, STL

### Basic Operations
1. **New Project**
   - Start with a clean workspace
   - Reset all views and settings

2. **Open Project**
   - Load existing `.ezy` files
   - Restore previous work

3. **Save Project**
   - Save current work to `.ezy` file
   - Auto-save feature available

4. **Import/Export**
   - Import external CAD files
   - Export to standard formats

## Modeling Tools

### 2D Sketching
1. **Basic Tools**
   - Add nodes
   - Create line edges
   - Draw multi-line edges
   - Add arc segments
   - Create circles
   - Draw rectangles and squares
   - Add slots

2. **Sketch Operations**
   - Define operation axis
   - Toggle edge dimensions
   - Mirror sketches
   - Create from face

### 3D Modeling
1. **Transform Operations**
   - Move shapes (G)
   - Rotate objects
   - Scale elements
   - Polar duplicate

2. **Feature Operations**
   - Extrude sketches (E)
   - Create chamfers
   - Add fillets
   - Boolean operations:
     - Cut
     - Fuse
     - Common

## Keyboard Shortcuts

### General Operations
- `Ctrl+O`: Open file
- `Ctrl+S`: Save file
- `Ctrl+Shift+S`: Save as
- `Esc`: Cancel current operation
- `Enter`: Confirm current operation
- `Tab`: Dimension input
- `Delete`: Remove selected elements

### Modeling Shortcuts
- `G`: Move mode
- `E`: Extrude mode
- `D`: Delete selected
- `L`: (Reserved for future use)

## View Controls

### Mouse Controls
- **Left Click**: Select objects
- **Right Click**: Context menu
- **Middle Click**: Pan view
- **Scroll Wheel**: Zoom in/out

### View Options
- Reset view
- Fit to screen
- Toggle wireframe
- Change material appearance
- Adjust lighting

## Tips and Tricks

### Efficient Modeling
1. Use keyboard shortcuts for common operations
2. Utilize the toolbar for quick access to tools
3. Take advantage of the dimension input feature
4. Use the log window to track operations

### Troubleshooting
1. Check the log window for error messages
2. Verify file permissions for save operations
3. Ensure sufficient system resources
4. Update graphics drivers if experiencing display issues

### Performance Optimization
1. Close unused sketches and shapes
2. Use wireframe mode for complex models
3. Optimize view settings for your hardware
4. Regular save operations to prevent data loss

## Support

### Documentation
- This usage guide
- Online documentation
- Video tutorials

### Community
- User forums
- GitHub repository
- Issue tracking

### Updates
- Regular feature updates
- Bug fixes
- Performance improvements

## Tool Icons

### Basic Operations
- ![User](icons/User.png) - Inspection mode
- ![Assembly_AxialMove](icons/Assembly_AxialMove.png) - Shape move (G)
- ![Draft_Rotate](icons/Draft_Rotate.png) - Shape rotate
- ![Part_Scale](icons/Part_Scale.png) - Shape scale

### Sketch Tools
- ![Workbench_Sketcher_none](icons/Workbench_Sketcher_none.png) - Sketch inspection mode
- ![Macro_FaceToSketch_48](icons/Macro_FaceToSketch_48.png) - Create sketch from face
- ![Sketcher_MirrorSketch](icons/Sketcher_MirrorSketch.png) - Define operation axis
- ![Sketcher_CreatePoint](icons/Sketcher_CreatePoint.png) - Add node
- ![Sketcher_Element_Line_Edge](icons/Sketcher_Element_Line_Edge.png) - Add line edge
- ![ls](icons/ls.png) - Add multi-line edge
- ![Sketcher_Element_Arc_Edge](icons/Sketcher_Element_Arc_Edge.png) - Add arc circle
- ![Sketcher_CreateSquare](icons/Sketcher_CreateSquare.png) - Add square
- ![Sketcher_CreateRectangle](icons/Sketcher_CreateRectangle.png) - Add rectangle from two points
- ![Sketcher_CreateRectangle_Center](icons/Sketcher_CreateRectangle_Center.png) - Add rectangle with center point
- ![Sketcher_CreateCircle](icons/Sketcher_CreateCircle.png) - Add circle
- ![Sketcher_Create3PointCircle](icons/Sketcher_Create3PointCircle.png) - Add circle from three points
- ![Sketcher_CreateSlot](icons/Sketcher_CreateSlot.png) - Add slot
- ![TechDraw_LengthDimension](icons/TechDraw_LengthDimension.png) - Toggle edge dimension annotation

### 3D Operations
- ![Design456_Extrude](icons/Design456_Extrude.png) - Extrude sketch face (E)
- ![PartDesign_Chamfer](icons/PartDesign_Chamfer.png) - Chamfer
- ![PartDesign_Fillet](icons/PartDesign_Fillet.png) - Fillet
- ![Draft_PolarArray](icons/Draft_PolarArray.png) - Shape polar duplicate

### Boolean Operations
- ![Part_Cut](icons/Part_Cut.png) - Shape cut
- ![Part_Fuse](icons/Part_Fuse.png) - Shape fuse
- ![Part_Common](icons/Part_Common.png) - Shape common

---

For more information, visit the official EzyCad website or GitHub repository.
