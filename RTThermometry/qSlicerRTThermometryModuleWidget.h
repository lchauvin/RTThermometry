/*==============================================================================

  Program: 3D Slicer

  Portions (c) Copyright Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

==============================================================================*/

#ifndef __qSlicerRTThermometryModuleWidget_h
#define __qSlicerRTThermometryModuleWidget_h

#include "vtkIGTLToMRMLImage.h"
#include "vtkImageData.h"
#include "vtkLookupTable.h"
#include "vtkMatrix4x4.h"
#include "vtkMRMLColorTableNode.h"
#include "vtkMRMLIGTLConnectorNode.h"
#include "vtkMRMLInteractionNode.h"
#include "vtkMRMLMarkupsDisplayNode.h"
#include "vtkMRMLMarkupsFiducialNode.h"
#include "vtkMRMLScalarVolumeDisplayNode.h"
#include "vtkMRMLScalarVolumeNode.h"
#include "vtkMRMLSelectionNode.h"
#include "vtkPointData.h"

// SlicerQt includes
#include "qSlicerAbstractModuleWidget.h"
#include "qSlicerRTThermometryModuleExport.h"
#include "qSlicerRTThermometryGraphWidget.h"

// VTK includes
#include <ctkVTKObject.h>

class qSlicerRTThermometryModuleWidgetPrivate;
class vtkMRMLNode;

/// \ingroup Slicer_QtModules_ExtensionTemplate
class Q_SLICER_QTMODULES_RTTHERMOMETRY_EXPORT qSlicerRTThermometryModuleWidget :
  public qSlicerAbstractModuleWidget
{
  Q_OBJECT
  QVTK_OBJECT

public:

  typedef qSlicerAbstractModuleWidget Superclass;
  qSlicerRTThermometryModuleWidget(QWidget *parent=0);
  virtual ~qSlicerRTThermometryModuleWidget();

public slots:
  void onServerRadioToggled(bool checked);
  void onConnectClicked();
  void onSetBaselineClicked();
  void onStatusConnected();
  void onStatusDisconnected();
  void onAddSensorClicked(bool pressed);
  void onRemoveSensorClicked();
  void onShowGraphChanged(int state);
  void onMarkupNodeAdded();
  void onMarkupNodeModified(vtkObject* vtkNotUsed(caller), vtkObject* callData);
  void onMarkupNodeRemoved();
  void onSensorChanged(int row, int column);
  void onPhaseImageModified();
  void onGraphHidden();

protected:
  QScopedPointer<qSlicerRTThermometryModuleWidgetPrivate> d_ptr;
  
  virtual void setup();
  void updateMarkupInWidget(Markup* modifiedMarkup);
  int getMarkupIndexByID(const char* markupID);
  void computePhaseDifference(vtkImageData* im1, vtkImageData* im2);
  void newImageAdded();
  void updateAllMarkups();
  void updateTemperatureGraph(int position, Markup* sensor);
  void createViewerNode();

private:
  Q_DECLARE_PRIVATE(qSlicerRTThermometryModuleWidget);
  Q_DISABLE_COPY(qSlicerRTThermometryModuleWidget);
};

#endif
