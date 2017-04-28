﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using CollectorsFrontEnd.InterfaceModels;
using System.Collections.ObjectModel;
using CollectorsFrontEnd.Interfaces.Subs;
using System.ComponentModel;

namespace CollectorsFrontEnd.Interfaces
{
    /// <summary>
    /// Interaction logic for CompCollectionView.xaml
    /// </summary>
    public partial class View_Collection : UserControl, IMenuBarComponent, INotifyPropertyChanged
    {
        #region DataBinding
        private UserControl _ImageComponent;
        public UserControl ImageComponent
        {
            get
            {
                return _ImageComponent;
            }

            set
            {
                _ImageComponent = value;
                OnPropertyChanged("ImageComponent");
            }
        }

        private readonly ObservableCollection<Module_Generalization> _LstGeneralizations = new ObservableCollection<Module_Generalization>();
        public ObservableCollection<Module_Generalization> LstGeneralizations
        {
            get
            {
                return _LstGeneralizations;
            }
        }

        protected virtual void OnPropertyChanged(string propertyName)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }
        #endregion

        #region Public Fields
        public event ComponentEvent UnhandledEvent;
        public event PropertyChangedEventHandler PropertyChanged;

        public CollectionModel DataModel;
        #endregion

        #region Private Fields
        private UserControl m_OverlayControl;
        #endregion

        #region Public Functions
        public View_Collection(string aszCollectionName)
        {
            InitializeComponent();
            DataContext = this;
            if ((DataModel = ServerInterfaceModel.GetCollectionModel(aszCollectionName)) == null)
            {
                DataModel = ServerInterfaceModel.GenerateCollectionModel(aszCollectionName);
            }

            buildListView();
        }

        public IDataModel GetDataModel()
        {
            return DataModel;
        }

        public List<Tuple<string, MenuAction>> GetMenuActions()
        {
            List<Tuple<string, MenuAction>> LstMenuActions = new List<Tuple<string, MenuAction>>()
            {
                new Tuple<string, MenuAction>("Set Baseline History", eSetBaselineHistory)
            };
            return LstMenuActions;
        }
        #endregion

        #region Private Functions
        private void buildListView()
        {
            if (!(DataModel.LstCopyModels.Count > 0))
            {
                return;
            }
            LstGeneralizations.Clear();
            Dictionary<string, List<CardModel>> mapGeneralizations = new Dictionary<string, List<CardModel>>();
            mapGeneralizations.Add("Main", new List<CardModel>());
            foreach (CardModel CM in DataModel.LstCopyModels)
            {
                string szGeneralization = CM.GetMetaTag("Generalization");
                if (szGeneralization != "")
                {
                    if (mapGeneralizations.ContainsKey(szGeneralization))
                    {
                        mapGeneralizations[szGeneralization].Add(CM);
                    }
                    else
                    {
                        mapGeneralizations.Add(szGeneralization, new List<CardModel>() { CM });
                    }
                }
                else
                {
                    mapGeneralizations["Main"].Add(CM);
                }
            }

            foreach (string szGeneralization in mapGeneralizations.Keys)
            {
                Module_Generalization CSG = new Module_Generalization(mapGeneralizations[szGeneralization], szGeneralization);
                CSG.UnhandledEvent += RouteReceivedUnhandledEvent;
                LstGeneralizations.Add(CSG);
            }

            updateCardDisplay(DataModel.LstCopyModels[0]);
        }

        private void showAddItemWindow()
        {
            showMainDisplay();
            if (DataModel.CollectionName != "")
            {
                Module_AddItemWindow ITI = new Module_AddItemWindow();
                m_OverlayControl = ITI;
                ITI.UnhandledEvent += RouteReceivedUnhandledEvent;
                Panel.SetZIndex(CenterPanel, 2);
                CenterPanel.Children.Add(ITI);
                SPItemsControl.IsEnabled = false;
            }
        }

        private void showBulkEditWindow()
        {
            showMainDisplay();
            if (DataModel.CollectionName != "")
            {
                Module_BulkEdits ITI = new Module_BulkEdits(DataModel);
                m_OverlayControl = ITI;
                ITI.UnhandledEvent += RouteReceivedUnhandledEvent;
                Panel.SetZIndex(CenterPanel, 2);
                CenterPanel.Children.Add(ITI);
                SPItemsControl.IsEnabled = false;
            }
        }

        private void showItemInterchangerWindow(List<CardModel> alstCardModels)
        {
            showMainDisplay();
            if (DataModel.CollectionName != "" && alstCardModels.Count > 0)
            {
                Module_AmountInterchanger ITI = new Module_AmountInterchanger(alstCardModels);
                m_OverlayControl = ITI;
                ITI.UnhandledEvent += RouteReceivedUnhandledEvent;
                Panel.SetZIndex(CenterPanel, 2);
                CenterPanel.Children.Add(ITI);
                SPItemsControl.IsEnabled = false;
            }
        }

        private void showAttrChangerWindow(CardModel aDataModel)
        {
            showMainDisplay();
            if (DataModel.CollectionName != "")
            {
                Module_AttributeChanger ITI = new Module_AttributeChanger(aDataModel);
                m_OverlayControl = ITI;
                ITI.UnhandledEvent += RouteReceivedUnhandledEvent;
                Panel.SetZIndex(CenterPanel, 2);
                CenterPanel.Children.Add(ITI);
                SPItemsControl.IsEnabled = false;
            }
        }

        private void updateCardDisplay(CardModel aCM)
        {
            ImageComponent = new Module_CardDisplayer(aCM);
        }

        private void showMainDisplay()
        {
            CenterPanel.Children.Remove(m_OverlayControl);
            m_OverlayControl = null;
            SPItemsControl.IsEnabled = true;
        }

        #endregion Private Functions

        #region Event Handlers
        private void ecBulkEditsAccept(Module_BulkEdits.Data aDataModel)
        {
            DataModel.SubmitBulkEdits(aDataModel.LstTextChanges);
            DataModel.Refresh();
            buildListView();
            showMainDisplay();
        }

        private void ecAddItem(Module_AddItemWindow.AddItemDataModel aDataModel)
        {
            //MainWindow.SCI.AddItem(ActiveCollection, m_CurrentAddItemWindow.ComboText, new List<Tuple<string, string>>());
            DataModel.AddItem(aDataModel.ComboBoxText, new List<Tuple<string, string>>());
            DataModel.Refresh();
            buildListView();
            ecAddItemWindowClose();
        }

        private void ecAddItemWindowClose()
        {
            showMainDisplay();
        }

        private void ecAmountInterchangerWindowOpen(CardModel aDataObject)
        {
            List<CardModel> lstCards = new List<CardModel>();
            List<string> lstGens = new List<string>();
            foreach (CardModel CM in DataModel.LstCopyModels)
            {
                List<Tuple<string, string>> LstIgnoreGeneralizationMetaTags = new List<Tuple<string, string>>()
                {
                    new Tuple<string, string>("_ignore","Generalization")
                };
                LstIgnoreGeneralizationMetaTags = LstIgnoreGeneralizationMetaTags.Concat(aDataObject.LstMetaTags).ToList();

                List<Tuple<string, string>> LstIgnoreGeneralizationMetaTagsTwo = new List<Tuple<string, string>>()
                {
                    new Tuple<string, string>("_ignore","Generalization")
                };
                LstIgnoreGeneralizationMetaTagsTwo = LstIgnoreGeneralizationMetaTagsTwo.Concat(CM.LstMetaTags).ToList();
                // Look for all the cards with matching "Long Name" and metatags then build an amount changer.
                if (!lstGens.Contains(CM.GetMetaTag("Generalization")) && CM.IsSameAs(aDataObject) && CM.IsSameMetaTags(LstIgnoreGeneralizationMetaTagsTwo, LstIgnoreGeneralizationMetaTags))
                {
                    lstGens.Insert(0, CM.GetMetaTag("Generalization"));
                    lstCards.Add(CM);
                }
            }
            showItemInterchangerWindow(lstCards);
        }

        private void ecAmountInterchangerWindowAccept(Module_AmountInterchanger.AmountInterchangerModel aDataModel)
        {
            List<Module_AmountChanger> ListChanges = aDataModel.LstGeneralizations;

            foreach (Module_AmountChanger AI in ListChanges)
            {
                Module_AmountChanger.Data oAmountChangerModel =
                    (Module_AmountChanger.Data)AI.GetDataModel();
                int iChangeCount = 0;
                if ((iChangeCount = (oAmountChangerModel.CurrentAmount - oAmountChangerModel.StartAmount)) > 0)
                {
                    for (int i = 0; i < iChangeCount; i++)
                    {
                        DataModel.AddItem(aDataModel.Copies[0].CardNameLong, oAmountChangerModel.LstMetaTags);
                    }

                }
                else
                {
                    iChangeCount = -iChangeCount;
                    for (int i = 0; i < iChangeCount; i++)
                    {
                        DataModel.RemoveItem(aDataModel.Copies[0].CardNameLong, oAmountChangerModel.LstMetaTags);
                    }
                }
            }

            DataModel.Refresh();
            buildListView();
            showMainDisplay();
        }

        private void ecAmountInterchangerWindowClose()
        {
            showMainDisplay();
        }

        private void ecAttrChangerWindowOpen(CardModel aDataObject)
        {
            showAttrChangerWindow(aDataObject);
        }

        private void ecAttrChangerWindowClose()
        {
            showMainDisplay();
        }

        private void ecAttrChangerWindowAccept(Module_AttributeChanger.CompSubAttributeChangerModel aDataModel)
        {
            // Calculate differences in meta tags
            // Calculate added tags
            List<Tuple<string, string>> LstAddedTags = new List<Tuple<string, string>>();
            foreach (var NewTup in aDataModel.LstCurrentMetaTags)
            {
                bool bFound = false;
                foreach (Tuple<string, string> Tup in aDataModel.CardModelObject.LstMetaTags)
                {
                    if (Tup.Item1 == NewTup.First && Tup.Item2 == NewTup.Second)
                    {
                        bFound = true;
                        break;
                    }
                }

                if (!bFound)
                {
                    LstAddedTags.Add(new Tuple<string, string>(NewTup.First, NewTup.Second));
                }
            }

            // Calculate removed tags
            List<Tuple<string, string>> LstRemovedTags = new List<Tuple<string, string>>();
            foreach (Tuple<string, string> OldTup in aDataModel.CardModelObject.LstMetaTags)
            {
                bool bFound = false;
                foreach (var Tup in aDataModel.LstCurrentMetaTags)
                {
                    if (Tup.First == OldTup.Item1)
                    {
                        bFound = true;
                        break;
                    }
                }

                if (!bFound)
                {
                    LstRemovedTags.Add(OldTup);
                }
            }

            List<Tuple<string, string>> LstMetaChanges = new List<Tuple<string, string>>();
            foreach (Tuple<string, string> AddTag in LstAddedTags)
            {
                LstMetaChanges.Add(AddTag);
            }

            foreach (Tuple<string, string> RemoveTag in LstRemovedTags)
            {
                // aDataModel.CardModelObject.RemoveMetaTag(RemoveTag.Item1);
                Tuple<string, string> RemoveTuple = new Tuple<string, string>(RemoveTag.Item1, "!NULL");
                LstMetaChanges.Add(RemoveTuple);
            }

            // Now do the attrs
            List<Tuple<string, string>> LstFinalAttrs = aDataModel.LstNonUniqueAttrs
                .Select(x => new Tuple<string, string>(x.Key, x.Value)).ToList();

            // Submit everything to the server.
            aDataModel.CardModelObject.SubmitFeatureChangesToServer(LstMetaChanges, LstFinalAttrs);
            DataModel.Refresh();
            buildListView();

            showMainDisplay();
        }

        private void ecGeneralizationSelectionChange(Module_Generalization.Data aDataObject)
        {
            foreach (Module_Generalization Gen in LstGeneralizations)
            {
                if (Gen.GeneralizationName != aDataObject.GeneralizationName)
                {
                    Gen.LVItems.UnselectAll();
                }
            }

            updateCardDisplay(aDataObject.SelectedItemDisplayer.DataModel);
        }

        public void RouteReceivedUnhandledEvent(IDataModel aDataObject, string aszAction)
        {

            if (aDataObject.GetType() == typeof(Module_AddItemWindow.AddItemDataModel))
            {
                Module_AddItemWindow.AddItemDataModel oDataModel =
                    (Module_AddItemWindow.AddItemDataModel)aDataObject;
                if (aszAction == "AddItem")
                {
                    ecAddItem(oDataModel);
                }
                else if (aszAction == "Cancel")
                {
                    ecAddItemWindowClose();
                }
            }
            // From One of the Entries in a Generalization (ItemDisplayer)
            else if (aDataObject.GetType() == typeof(CardModel))
            {
                if (aszAction == "Gen.DeltaAmtOpen")
                {
                    ecAmountInterchangerWindowOpen((CardModel)aDataObject);
                }
                else if (aszAction == "Gen.AttrChangerOpen")
                {
                    ecAttrChangerWindowOpen((CardModel)aDataObject);
                }
            }
            // From the attr changer
            else if (aDataObject.GetType() == typeof(Module_AttributeChanger.CompSubAttributeChangerModel))
            {
                Module_AttributeChanger.CompSubAttributeChangerModel oDataModel =
                    (Module_AttributeChanger.CompSubAttributeChangerModel)aDataObject;
                if (aszAction == "Cancel")
                {
                    ecAttrChangerWindowClose();
                }
                else if (aszAction == "OK")
                {
                    ecAttrChangerWindowAccept(oDataModel);
                }
            }
            // From The AmountInterchanger
            else if (aDataObject.GetType() == typeof(Module_AmountInterchanger.AmountInterchangerModel))
            {
                if (aszAction == "Close")
                {
                    ecAmountInterchangerWindowClose();
                }
                else if (aszAction == "OK")
                {
                    ecAmountInterchangerWindowAccept((Module_AmountInterchanger.AmountInterchangerModel)aDataObject);
                }
            }
            else if (aDataObject.GetType() == typeof(Module_Generalization.Data))
            {
                if (aszAction == "SelectionChanged")
                {
                    ecGeneralizationSelectionChange((Module_Generalization.Data)aDataObject);
                }
            }
            else if (aDataObject.GetType() == typeof(Module_BulkEdits.Data))
            {
                Module_BulkEdits.Data dM = (Module_BulkEdits.Data)aDataObject;
                if (aszAction == "Cancel")
                {
                    showMainDisplay();
                }
                else if (aszAction == "Accept")
                {
                    ecBulkEditsAccept(dM);
                }
            }
        }
        #endregion

        #region UI Event Handlers
        public void eSetBaselineHistory()
        {
            DataModel.SetBaselineHistory();
        }

        public void eAddItemWindowButton_Click(object sender, RoutedEventArgs e)
        {
            showAddItemWindow();
        }

        private void eSaveCollection_Click(object sender, RoutedEventArgs e)
        {
            DataModel.SaveCollection();
        }

        private void eBulkEditWindow_Click(object sender, RoutedEventArgs e)
        {
            showBulkEditWindow();
        }
        #endregion

    }
}
