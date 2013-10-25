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

// Qt includes
#include <QtPlugin>

// RTThermometry Logic includes
#include <vtkSlicerRTThermometryLogic.h>

// RTThermometry includes
#include "qSlicerRTThermometryModule.h"
#include "qSlicerRTThermometryModuleWidget.h"

//-----------------------------------------------------------------------------
Q_EXPORT_PLUGIN2(qSlicerRTThermometryModule, qSlicerRTThermometryModule);

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_ExtensionTemplate
class qSlicerRTThermometryModulePrivate
{
public:
  qSlicerRTThermometryModulePrivate();
};

//-----------------------------------------------------------------------------
// qSlicerRTThermometryModulePrivate methods

//-----------------------------------------------------------------------------
qSlicerRTThermometryModulePrivate
::qSlicerRTThermometryModulePrivate()
{
}

//-----------------------------------------------------------------------------
// qSlicerRTThermometryModule methods

//-----------------------------------------------------------------------------
qSlicerRTThermometryModule
::qSlicerRTThermometryModule(QObject* _parent)
  : Superclass(_parent)
  , d_ptr(new qSlicerRTThermometryModulePrivate)
{
}

//-----------------------------------------------------------------------------
qSlicerRTThermometryModule::~qSlicerRTThermometryModule()
{
}

//-----------------------------------------------------------------------------
QString qSlicerRTThermometryModule::helpText()const
{
  return "This is a loadable module bundled in an extension";
}

//-----------------------------------------------------------------------------
QString qSlicerRTThermometryModule::acknowledgementText()const
{
  return "This work was was partially funded by NIH grant 3P41RR013218-12S1";
}

//-----------------------------------------------------------------------------
QStringList qSlicerRTThermometryModule::contributors()const
{
  QStringList moduleContributors;
  moduleContributors << QString("Laurent Chauvin (BWH)");
  return moduleContributors;
}

//-----------------------------------------------------------------------------
QIcon qSlicerRTThermometryModule::icon()const
{
  return QIcon(":/Icons/RTThermometry.png");
}

//-----------------------------------------------------------------------------
QStringList qSlicerRTThermometryModule::categories() const
{
  return QStringList() << "IGT";
}

//-----------------------------------------------------------------------------
QStringList qSlicerRTThermometryModule::dependencies() const
{
  return QStringList();
}

//-----------------------------------------------------------------------------
void qSlicerRTThermometryModule::setup()
{
  this->Superclass::setup();
}

//-----------------------------------------------------------------------------
qSlicerAbstractModuleRepresentation * qSlicerRTThermometryModule
::createWidgetRepresentation()
{
  return new qSlicerRTThermometryModuleWidget;
}

//-----------------------------------------------------------------------------
vtkMRMLAbstractLogic* qSlicerRTThermometryModule::createLogic()
{
  return vtkSlicerRTThermometryLogic::New();
}
