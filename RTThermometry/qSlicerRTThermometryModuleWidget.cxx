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
#include <QDebug>

// SlicerQt includes
#include "qSlicerRTThermometryModuleWidget.h"
#include "ui_qSlicerRTThermometryModuleWidget.h"

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_ExtensionTemplate
class qSlicerRTThermometryModuleWidgetPrivate: public Ui_qSlicerRTThermometryModuleWidget
{
public:
  vtkMRMLIGTLConnectorNode* IGTLConnector;
  vtkMRMLMarkupsFiducialNode* SensorList;
  vtkMRMLScalarVolumeNode* FirstPhaseImageNode;
  vtkMRMLScalarVolumeNode* PhaseImageNode;
  vtkMRMLScalarVolumeNode* ViewerNode;
  vtkMatrix4x4* RASToIJK;
  std::vector<vtkImageData*> TemperatureImageList;

  vtkMRMLSelectionNode* SelectionNode;
  vtkMRMLInteractionNode* InteractionNode;

public:
  qSlicerRTThermometryModuleWidgetPrivate();
  ~qSlicerRTThermometryModuleWidgetPrivate();
};

//-----------------------------------------------------------------------------
// qSlicerRTThermometryModuleWidgetPrivate methods

//-----------------------------------------------------------------------------
qSlicerRTThermometryModuleWidgetPrivate::qSlicerRTThermometryModuleWidgetPrivate()
{
  this->IGTLConnector = NULL;
  this->SensorList = NULL;
  this->FirstPhaseImageNode = NULL;
  this->PhaseImageNode = NULL;
  this->ViewerNode = NULL;
  this->RASToIJK = NULL;

  this->SelectionNode = NULL;
  this->InteractionNode = NULL;
}

//-----------------------------------------------------------------------------
qSlicerRTThermometryModuleWidgetPrivate::~qSlicerRTThermometryModuleWidgetPrivate()
{
  if (this->PhaseImageNode && this->IGTLConnector)
    {
    this->IGTLConnector->UnregisterIncomingMRMLNode(this->PhaseImageNode);
    this->IGTLConnector->Delete();
    }

  if (this->SensorList)
    {
    this->SensorList->Delete();
    }

  if (this->FirstPhaseImageNode)
    {
    this->FirstPhaseImageNode->Delete();
    }

  if (this->RASToIJK)
    {
    this->RASToIJK->Delete();
    }
}

//-----------------------------------------------------------------------------
// qSlicerRTThermometryModuleWidget methods

//-----------------------------------------------------------------------------
qSlicerRTThermometryModuleWidget::qSlicerRTThermometryModuleWidget(QWidget* _parent)
  : Superclass( _parent )
  , d_ptr( new qSlicerRTThermometryModuleWidgetPrivate )
{
}

//-----------------------------------------------------------------------------
qSlicerRTThermometryModuleWidget::~qSlicerRTThermometryModuleWidget()
{
  this->qvtkDisconnectAll();
}

//-----------------------------------------------------------------------------
void qSlicerRTThermometryModuleWidget::setup()
{
  Q_D(qSlicerRTThermometryModuleWidget);
  d->setupUi(this);
  this->Superclass::setup();

  // Settings
  connect(d->ServerRadio, SIGNAL(toggled(bool)),
	  this, SLOT(onServerRadioToggled(bool)));

  connect(d->ConnectButton, SIGNAL(clicked()),
	  this, SLOT(onConnectClicked()));

  // Sensors
  if (d->SensorTableWidget)
    {
    // Hide ID column
    d->SensorTableWidget->setColumnHidden(0,true);
    }

  connect(d->AddSensorButton, SIGNAL(toggled(bool)),
	  this, SLOT(onAddSensorClicked(bool)));

  connect(d->RemoveSensorButton, SIGNAL(clicked()),
	  this, SLOT(onRemoveSensorClicked()));

  connect(d->ShowGraphCheckbox, SIGNAL(stateChanged(int)),
	  this, SLOT(onShowGraphChanged(int)));

  connect(d->SensorTableWidget, SIGNAL(cellChanged(int,int)),
	  this, SLOT(onSensorChanged(int,int)));

  // Time Player
}

//-----------------------------------------------------------------------------
void qSlicerRTThermometryModuleWidget::onServerRadioToggled(bool checked)
{
  Q_D(qSlicerRTThermometryModuleWidget);

  if (!d->HostnameLine || !d->PortLine)
    {
    return;
    }

  if (checked)
    {
    d->HostnameLine->setEnabled(false);
    }
  else
    {
    d->HostnameLine->setEnabled(true);
    }
}

//-----------------------------------------------------------------------------
void qSlicerRTThermometryModuleWidget::onConnectClicked()
{
  Q_D(qSlicerRTThermometryModuleWidget);

  if (!this->mrmlScene())
    {
    return;
    }

  // Create Markup node
  if (!d->SensorList)
    {
    d->SensorList = vtkMRMLMarkupsFiducialNode::New();
    d->SensorList->SetName("Sensors");
    this->mrmlScene()->AddNode(d->SensorList);
    vtkMRMLMarkupsDisplayNode* displayNode =
      vtkMRMLMarkupsDisplayNode::New();
    displayNode->SetGlyphType(vtkMRMLMarkupsDisplayNode::Cross2D);
    displayNode->SetGlyphScale(4.0);
    this->mrmlScene()->InsertBeforeNode(d->SensorList, displayNode);
    d->SensorList->DisableModifiedEventOn();
    d->SensorList->AddAndObserveDisplayNodeID(displayNode->GetID());
    d->SensorList->DisableModifiedEventOff();
    displayNode->Delete();

    this->qvtkConnect(d->SensorList, vtkMRMLMarkupsNode::MarkupAddedEvent,
		      this, SLOT(onMarkupNodeAdded()));
    this->qvtkConnect(d->SensorList, vtkMRMLMarkupsNode::PointModifiedEvent,
		      this, SLOT(onMarkupNodeModified(vtkObject*, vtkObject*)));
    this->qvtkConnect(d->SensorList, vtkMRMLMarkupsNode::NthMarkupModifiedEvent,
		      this, SLOT(onMarkupNodeModified(vtkObject*, vtkObject*)));
    this->qvtkConnect(d->SensorList, vtkMRMLMarkupsNode::MarkupRemovedEvent,
		      this, SLOT(onMarkupNodeRemoved()));
    }
  
  // Add OpenIGTLConnector node 
  if (!d->IGTLConnector)
    {
    d->IGTLConnector = vtkMRMLIGTLConnectorNode::New();
    this->mrmlScene()->AddNode(d->IGTLConnector);

    this->qvtkConnect(d->IGTLConnector, vtkMRMLIGTLConnectorNode::ConnectedEvent,
		      this, SLOT(onStatusConnected()));
    this->qvtkConnect(d->IGTLConnector, vtkMRMLIGTLConnectorNode::DisconnectedEvent,
		      this, SLOT(onStatusDisconnected()));
    }
  
  // Connect
  if (d->ServerRadio->isChecked())
    {
    // Server type
    d->IGTLConnector->SetTypeServer(d->PortLine->text().toDouble());
    }
  else
    {
    // Client type
    d->IGTLConnector->SetTypeClient(d->HostnameLine->text().toStdString(),
				       d->PortLine->text().toDouble());
    }
  d->IGTLConnector->Start();
  d->ConnectionFrame->setText("Connection - Waiting for connection...");
}

//-----------------------------------------------------------------------------
void qSlicerRTThermometryModuleWidget::onStatusConnected()
{
  Q_D(qSlicerRTThermometryModuleWidget);

  if (!d->ConnectionFrame || !d->IGTLConnector || !this->mrmlScene())
    {
    return;
    }

  d->ConnectionFrame->setCollapsed(true);

  // Phase Image Node
  vtkSmartPointer<vtkIGTLToMRMLImage> imageConverter =
    vtkSmartPointer<vtkIGTLToMRMLImage>::New();
  if (!d->PhaseImageNode)
    {
    if (imageConverter)
      {
      d->PhaseImageNode =
	vtkMRMLScalarVolumeNode::SafeDownCast(imageConverter->CreateNewNode(this->mrmlScene(), "ImagerClient"));
      if (d->PhaseImageNode)
	{
	d->IGTLConnector->RegisterIncomingMRMLNode(d->PhaseImageNode);
	this->qvtkConnect(d->PhaseImageNode, vtkMRMLVolumeNode::ImageDataModifiedEvent,
			  this, SLOT(onPhaseImageModified()));
	}
      }
    }

  // Viewer Node
  if (!d->ViewerNode)
    {
    d->ViewerNode =
      vtkMRMLScalarVolumeNode::SafeDownCast(imageConverter->CreateNewNode(this->mrmlScene(), "TemperatureViewer"));
    }

  // RAS To IJK Matrix
  if (!d->RASToIJK)
    {
    d->RASToIJK = vtkMatrix4x4::New();
    }
  
  d->ConnectionFrame->setText("Connection - Connected");
  d->IGTLConnector->RegisterIncomingMRMLNode(d->PhaseImageNode);
}

//-----------------------------------------------------------------------------
void qSlicerRTThermometryModuleWidget::onStatusDisconnected()
{
  Q_D(qSlicerRTThermometryModuleWidget);

  if (d->ConnectionFrame)
    {
    d->ConnectionFrame->setCollapsed(false);
    }
  d->ConnectionFrame->setText("Connection - Disconnected");
}

//-----------------------------------------------------------------------------
void qSlicerRTThermometryModuleWidget::onAddSensorClicked(bool pressed)
{
  Q_D(qSlicerRTThermometryModuleWidget);

  if (!this->mrmlScene() || !d->SensorList)
    {
    return;
    }

  if (!d->SelectionNode || !d->InteractionNode)
    {
    d->SelectionNode =
      vtkMRMLSelectionNode::SafeDownCast(this->mrmlScene()->GetNodeByID("vtkMRMLSelectionNodeSingleton"));
    d->InteractionNode =
      vtkMRMLInteractionNode::SafeDownCast(this->mrmlScene()->GetNodeByID("vtkMRMLInteractionNodeSingleton"));
    }

  if (!d->SelectionNode || !d->InteractionNode)
    {
    return;
    }
  
  d->SelectionNode->SetReferenceActivePlaceNodeClassName(d->SensorList->GetClassName());
  d->SelectionNode->SetActivePlaceNodeID(d->SensorList->GetID());

  if (pressed)
    {
    d->InteractionNode->SwitchToSinglePlaceMode();
    d->InteractionNode->SetCurrentInteractionMode(vtkMRMLInteractionNode::Place);
    }
  else
    {
    d->InteractionNode->SetCurrentInteractionMode(vtkMRMLInteractionNode::ViewTransform);
    }
}

//-----------------------------------------------------------------------------
void qSlicerRTThermometryModuleWidget::onRemoveSensorClicked()
{
  Q_D(qSlicerRTThermometryModuleWidget);

  if (!d->SensorTableWidget || !d->SensorList)
    {
    return;
    }

  int selectedRow = d->SensorTableWidget->currentRow();
  int numberOfRow = d->SensorTableWidget->rowCount();
  if (selectedRow < numberOfRow &&
      selectedRow >= 0)
    {
    QTableWidgetItem* itemID = d->SensorTableWidget->item(selectedRow,0);
    if (itemID)
      {
      std::string currentID(itemID->text().toStdString());
      int markupIndex = this->getMarkupIndexByID(currentID.c_str());
      if (markupIndex >= 0)
        {
        d->SensorList->RemoveMarkup(markupIndex);
        }
      }
    }
}

//-----------------------------------------------------------------------------
void qSlicerRTThermometryModuleWidget::onShowGraphChanged(int vtkNotUsed(state))
{
}

//-----------------------------------------------------------------------------
void qSlicerRTThermometryModuleWidget::onMarkupNodeAdded()
{
  Q_D(qSlicerRTThermometryModuleWidget);

  if (!d->SensorList)
    {
    return;
    }

  d->AddSensorButton->setChecked(false);
  Markup* lastMarkup = d->SensorList->GetNthMarkup(d->SensorList->GetNumberOfMarkups()-1);
  if (lastMarkup)
    {
    if (!lastMarkup->Label.empty())
      {
      lastMarkup->Description.assign(lastMarkup->Label);
      }
    }
}

//-----------------------------------------------------------------------------
void qSlicerRTThermometryModuleWidget::onMarkupNodeModified(vtkObject* vtkNotUsed(caller),
							    vtkObject* callData)
{
  Q_D(qSlicerRTThermometryModuleWidget);

  if (!callData || !d->SensorList)
    {
    return;
    }

  int *nPtr = NULL;
  int n = -1;
  nPtr = reinterpret_cast<int*>(callData);
  if (nPtr)
    {
    n = *nPtr;
    }

  if (n >= 0 && n < d->SensorList->GetNumberOfMarkups())
    {
    Markup* modifiedMarkup = d->SensorList->GetNthMarkup(n);
    if (modifiedMarkup)
      {
      this->updateMarkupInWidget(modifiedMarkup);
      }
    }
}

//-----------------------------------------------------------------------------
void qSlicerRTThermometryModuleWidget::onMarkupNodeRemoved()
{
  Q_D(qSlicerRTThermometryModuleWidget);

  if (!d->SensorList || !d->SensorTableWidget)
    {
    return;
    }

  // Clear list
  d->SensorTableWidget->clearContents();
  d->SensorTableWidget->setRowCount(0);

  // Re-populate the widget
  int numberOfMarkups = d->SensorList->GetNumberOfMarkups();
  for (int i = 0; i < numberOfMarkups; ++i)
    {
    Markup* tmpMarkup = d->SensorList->GetNthMarkup(i);
    this->updateMarkupInWidget(tmpMarkup);
    }
}

//-----------------------------------------------------------------------------
void qSlicerRTThermometryModuleWidget::onSensorChanged(int row, int column)
{
  Q_D(qSlicerRTThermometryModuleWidget);

  if (!d->SensorTableWidget || column != 1 || row >= d->SensorTableWidget->rowCount() ||
      !d->SensorList)
    {
    return;
    }

  std::string currentNodeID(d->SensorTableWidget->item(row,0)->text().toStdString());
  std::string newNodeName(d->SensorTableWidget->item(row,1)->text().toStdString());
  int markupIndex = this->getMarkupIndexByID(currentNodeID.c_str());
  if (markupIndex >= 0)
    {
    Markup* tmpMarkup = d->SensorList->GetNthMarkup(markupIndex);
    if (tmpMarkup)
      {
      if (tmpMarkup->Description.compare(newNodeName) != 0)
        {
        d->SensorList->SetNthMarkupDescription(markupIndex, newNodeName);
        }
      }
    }
}

//-----------------------------------------------------------------------------
void qSlicerRTThermometryModuleWidget::onPhaseImageModified()
{
  Q_D(qSlicerRTThermometryModuleWidget);
  
  if (!d->PhaseImageNode || !this->logic() || !d->RASToIJK ||
      !d->ViewerNode)
    {
    return;
    }

  if (!d->FirstPhaseImageNode)
    {
    vtkSmartPointer<vtkImageData> firstImageData = vtkSmartPointer<vtkImageData>::New();
    d->FirstPhaseImageNode = vtkMRMLScalarVolumeNode::New();
    d->FirstPhaseImageNode->Copy(d->PhaseImageNode);
    firstImageData->DeepCopy(d->PhaseImageNode->GetImageData());
    d->FirstPhaseImageNode->SetAndObserveImageData(firstImageData.GetPointer());

    d->ViewerNode->Copy(d->PhaseImageNode);
    return;
    }

  d->PhaseImageNode->GetRASToIJKMatrix(d->RASToIJK);

  // Require at least 2 images to compute phase difference
  if (d->FirstPhaseImageNode && d->PhaseImageNode)
    {
    this->computePhaseDifference(d->FirstPhaseImageNode, d->PhaseImageNode);
    }
}
//-----------------------------------------------------------------------------
void qSlicerRTThermometryModuleWidget::updateMarkupInWidget(Markup* modifiedMarkup)
{
  Q_D(qSlicerRTThermometryModuleWidget);

  if (!d->SensorTableWidget || !d->RASToIJK)
    {
    return;
    }

  // Find is markup has already been added
  int rowNumber = d->SensorTableWidget->rowCount();
  int itemIndex = -1;
  for (int i = 0; i < rowNumber; ++i)
    {
    QTableWidgetItem* currentItem = d->SensorTableWidget->item(i,0);
    if (modifiedMarkup->ID.compare(currentItem->text().toStdString()) == 0)
      {
      itemIndex = i;
      continue;
      }
    }

  if (itemIndex < 0)
    {
    // Markup is not in the list yet. Add it.
    QTableWidgetItem* idItem = new QTableWidgetItem();
    QTableWidgetItem* nameItem = new QTableWidgetItem();
    nameItem->setFlags(nameItem->flags() | Qt::ItemIsEditable);
    QTableWidgetItem* temperatureItem = new QTableWidgetItem();
    temperatureItem->setFlags(temperatureItem->flags() & ~Qt::ItemIsEditable);

    d->SensorTableWidget->insertRow(rowNumber);
    d->SensorTableWidget->setItem(rowNumber, 0, idItem);
    d->SensorTableWidget->setItem(rowNumber, 1, nameItem);
    d->SensorTableWidget->setItem(rowNumber, 2, temperatureItem);

    std::string markupID(modifiedMarkup->ID);
    d->SensorTableWidget->item(rowNumber,0)->setText(markupID.c_str());

    itemIndex = rowNumber;
    }

  // Update temperature
  double temp = 0.0;
  if (d->TemperatureImageList.size() > 0)
    {
    // Get Markup position
    double mPos[4] = { modifiedMarkup->points[0].X(),
		       modifiedMarkup->points[0].Y(),
		       modifiedMarkup->points[0].Z(),
		       1.0 };
    double mIJKPos[4];
    d->RASToIJK->MultiplyPoint(mPos, mIJKPos);

    vtkImageData* lastImage = 
      d->ViewerNode->GetImageData();
    if (lastImage)
      {
      temp = lastImage->GetScalarComponentAsDouble(mIJKPos[0], mIJKPos[1], mIJKPos[2], 0);
      }
    }
  d->SensorTableWidget->item(itemIndex, 2)->setText(QString::number(temp));

  // Update name
  std::stringstream markupName;
  markupName << modifiedMarkup->Description << " (" << temp << ")";
  modifiedMarkup->Label = markupName.str();
  d->SensorList->Modified();
  d->SensorTableWidget->item(itemIndex,1)->setText(modifiedMarkup->Description.c_str());
}

//-----------------------------------------------------------------------------
int qSlicerRTThermometryModuleWidget::
getMarkupIndexByID(const char* markupID)
{
  Q_D(qSlicerRTThermometryModuleWidget);
  
  if (!d->SensorList)
    {
    return -1;
    }

  int numberOfMarkups = d->SensorList->GetNumberOfMarkups();
  for (int i = 0; i < numberOfMarkups; ++i)
    {
    Markup* compareMarkup = d->SensorList->GetNthMarkup(i);
    if (compareMarkup &&
        strcmp(compareMarkup->ID.c_str(), markupID) == 0)
      {
      return i;
      }
    }
  return -1;
}

//-----------------------------------------------------------------------------
void qSlicerRTThermometryModuleWidget::
computePhaseDifference(vtkMRMLScalarVolumeNode* firstNode, vtkMRMLScalarVolumeNode* secondNode)
{
  clock_t start = clock();

  Q_D(qSlicerRTThermometryModuleWidget);

  if (!firstNode || !secondNode)
    {
    return;
    }

  vtkImageData* firstImData = firstNode->GetImageData();
  vtkImageData* secondImData = secondNode->GetImageData();

  if (d->TemperatureImageList.size() > 0)
    {
    firstImData = d->TemperatureImageList[d->TemperatureImageList.size()-1];
    }

  if (!firstImData || !secondImData ||
      firstImData == secondImData)
    {
    return;
    }

  vtkSmartPointer<vtkImageData> newImData = vtkSmartPointer<vtkImageData>::New();
  newImData->SetDimensions(firstImData->GetDimensions());
  newImData->SetSpacing(firstImData->GetSpacing());
  newImData->SetScalarType(firstImData->GetScalarType());
  newImData->SetNumberOfScalarComponents(1);
  newImData->AllocateScalars();

  unsigned char* im1BufferPointer = (unsigned char*) firstImData->GetPointData()->GetScalars()->GetVoidPointer(0);
  unsigned char* im2BufferPointer = (unsigned char*) secondImData->GetPointData()->GetScalars()->GetVoidPointer(0);
  unsigned char* outputBufferPointer = (unsigned char*) newImData->GetPointData()->GetScalars()->GetVoidPointer(0);

  int extent[6];
  newImData->GetExtent(extent);
  
  vtkIdType inc[3];
  newImData->GetIncrements(inc);

  for (int k=0; k<extent[5]+1; ++k)
    {
    for (int j=0; j<extent[3]+1; ++j)
      {
      for (int i=0; i<extent[1]+1; ++i)
	{
	unsigned char phaseDiff = im2BufferPointer[k*inc[2]+j*inc[1]+i] - im1BufferPointer[k*inc[2]+j*inc[1]+i];
	unsigned char temp = phaseDiff;/*37.0 + (phaseDiff) * M_PI / 4096 / 2.0 / 0.010 / (2*M_PI*42.576) / 3.0 / 0.010);*/
	outputBufferPointer[k*inc[2]+j*inc[1]+i] = temp;
	}
      }
    }
  d->TemperatureImageList.push_back(newImData.GetPointer());
  this->newImageAdded();
  
  clock_t end = clock();
  std::cerr << "Time: " << (float)(end-start)/CLOCKS_PER_SEC << std::endl;
}

//-----------------------------------------------------------------------------
void qSlicerRTThermometryModuleWidget::
newImageAdded()
{
  Q_D(qSlicerRTThermometryModuleWidget);

  if (d->ViewerNode)
    {
    d->ViewerNode->SetAndObserveImageData(d->TemperatureImageList[d->TemperatureImageList.size()-1]);
    }

  this->updateAllMarkups();

  this->saveMarkupsValues();
}

//-----------------------------------------------------------------------------
void qSlicerRTThermometryModuleWidget::
updateAllMarkups()
{
  Q_D(qSlicerRTThermometryModuleWidget);

  if (!d->SensorTableWidget || !d->SensorList)
    {
    return;
    }
 
  int numberOfMarkups = d->SensorTableWidget->rowCount();
  for (int i = 0; i < numberOfMarkups; ++i)
    {
    int n = this->getMarkupIndexByID(d->SensorTableWidget->item(i,0)->text().toStdString().c_str());
    if (n >= 0 && n < d->SensorList->GetNumberOfMarkups())
      {
      Markup* updateMarkup = d->SensorList->GetNthMarkup(n);
      if (updateMarkup)
	{
	this->updateMarkupInWidget(updateMarkup);
	}
      }
    }
}

//-----------------------------------------------------------------------------
void qSlicerRTThermometryModuleWidget::
saveMarkupsValues()
{
}
