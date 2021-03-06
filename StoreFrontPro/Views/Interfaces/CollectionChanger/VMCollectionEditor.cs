﻿using StoreFrontPro.Server;
using StoreFrontPro.Tools;
using StoreFrontPro.Views.Components.SuggestionsSearchBox;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading.Tasks;

namespace StoreFrontPro.Views.Interfaces.CollectionChanger
{
   class VMCollectionEditor : ViewModel<CollectionModel>, IViewComponent, IVCISupporter
   {
      #region Interface Names
      public const string AddSearchBox = "ASB";
      public const string RemoveSearchBox = "RSB";
      public const string CollectionEditorItem = "CEI";
      #endregion

      #region Data Binding
      public ObservableCollection<VCollectionEditorItem> TextChangesList { get; set; } =
         new ObservableCollection<VCollectionEditorItem>();

      public VSuggestionsSearchBox AddCardSearchBox { get; set; }
      public VSuggestionsSearchBox RemoveCardSearchBox { get; set; }

      public RelayCommand AcceptCommand { get; set; }
      public RelayCommand CancelCommand { get; set; }
      #endregion

      #region Properties
      private List<MCollectionEditorItem> m_lstItems { get; set; }

      public string AddCardText
      {
         get { return (AddCardSearchBox.DataContext as VMSuggestionsSearchBox).TextBoxValue; }
      }

      public string RemoveCardText
      {
         get { return (RemoveCardSearchBox.DataContext as VMSuggestionsSearchBox).TextBoxValue; }
      }
      #endregion

      #region Events
      public event DisplayEventHandler DisplayEvent;
      #endregion

      #region Public Functions
      public VMCollectionEditor(CollectionModel Model, string RoutingName) : base(Model, RoutingName)
      {
         AcceptCommand = new RelayCommand(eAcceptCommand);
         CancelCommand = new RelayCommand(eCancelCommand);
         m_lstItems = new List<MCollectionEditorItem>();

         ViewClass VCAdd = ViewFactory.CreateSuggestionSearchBox(
            SearchCollection: ServerInterface.Server.GetAllCardsStartingWith,
            ActionName: "Add Card",
            RoutingName: AddSearchBox);
         VCAdd.HookDisplayEvent(DisplayEventHandler);
         AddCardSearchBox = VCAdd.View as VSuggestionsSearchBox;

         ViewClass VCRemove = ViewFactory.CreateSuggestionSearchBox(
            SearchCollection: Model.SearchCollection,
            ActionName: "Remove/Replace Card",
            RoutingName: RemoveSearchBox);
         VCRemove.HookDisplayEvent(DisplayEventHandler);
         RemoveCardSearchBox = VCRemove.View as VSuggestionsSearchBox;
      }
      #endregion

      #region Private Methods
      private void addEditorItem(CollectionDelta aCDelta)
      {
         ViewClass VC = ViewFactory.CreateCollectionEditorItem(aCDelta, CollectionEditorItem);
         m_lstItems.Add((MCollectionEditorItem)VC.Model);
         TextChangesList.Add((VCollectionEditorItem)VC.View);
      }

      private List<string> getFunctionList()
      {
         return m_lstItems.Select(x => itemToCommand(x)).ToList();
      }

      private string itemToCommand(MCollectionEditorItem amcei)
      {
         string szCommandChar = amcei.FunctionText.Substring(0, 1);
         string szCount = " x" + amcei.Amount;
         string szId =  amcei.FunctionText.Substring(1);

         // if the item is an addition, include the set.
         // The set selection occurs after the card is decided, so this must be added here.
         if( szCommandChar == "+" || szCommandChar == "%")
         {
            szId += " { set=\"" + amcei.SelectedSet + "\" }";
         }

         return szCommandChar + szCount + szId;
      }

      private List<string> getIdentifierOptions(string szCard)
      {
         List<string> lstSetOptions;
         CardModel.GetPrototype(szCard).AttributeOptions.TryGetValue("set", out lstSetOptions);
         return lstSetOptions ?? new List<string>();
      }

      private string getStandardShortId(CardModel aCM)
      {
         return aCM.GetIdealIdentifier();
      }

      private void clearSearchBoxes()
      {
         VMSuggestionsSearchBox removeBoxVM = (RemoveCardSearchBox.DataContext as VMSuggestionsSearchBox);
         removeBoxVM.TextBoxValue = "";

         VMSuggestionsSearchBox addBoxVM = (AddCardSearchBox.DataContext as VMSuggestionsSearchBox);
         addBoxVM.TextBoxValue = "";
      }
      #endregion

      #region Event Handlers
      private void eSearchOKEventHandler(string aszSourceName)
      {
         if (aszSourceName == AddSearchBox)
         {
            eAddCardEventHandler();
         }
         else
         {
            eRemoveCardEventHandler();
         }
      }

      private void eRemoveCardEventHandler()
      {
         CollectionDelta delta = Model.GetDeltaCommand(AddCard: AddCardText, RemoveIdealIdentifier: RemoveCardText);
         clearSearchBoxes();
         if (delta != null)
         {
            addEditorItem(delta);
         }
      }

      private void eAddCardEventHandler()
      {
         CollectionDelta delta = Model.GetDeltaCommand(AddCard: AddCardText, RemoveIdealIdentifier: "");
         clearSearchBoxes();
         if (delta != null)
         {
            addEditorItem(delta);
         }
      }

      private void eAcceptCommand(object canExecute)
      {
         DisplayEventArgs eventArgs = new DisplayEventArgs(VCICollectionEditor.Accept);
         Model.SubmitBulkEdits(getFunctionList());
         DisplayEvent?.Invoke(this, eventArgs);
      }

      private void eCancelCommand(object canExecute)
      {
         DisplayEventArgs eventArgs = new DisplayEventArgs(VCICollectionEditor.Cancel);
         DisplayEvent?.Invoke(this, eventArgs);
      }
      #endregion

      #region IViewComponent
      public List<StoreFrontMenuItem> GetMenuItems()
      {
         return new List<StoreFrontMenuItem>();
      }
      #endregion

      #region IVCISupporter
      public void DisplayEventHandler(object source, DisplayEventArgs e)
      {
         GetRouter().Call(source.GetType(), this, e.Key, e.Args);
      }

      public InterfaceRouter GetRouter()
      {
         return IRouter;
      }

      static InterfaceRouter _IRouter = null;
      static InterfaceRouter IRouter
      {
         get
         {
            if (_IRouter == null) { BuildInterface(); }
            return _IRouter;
         }
      }

      static void BuildInterface()
      {
         _IRouter = new InterfaceRouter();

         VCISuggestionsSearchBox VCAddIS = new VCISuggestionsSearchBox(
            OK: (x) => { return (x as VMCollectionEditor).eSearchOKEventHandler; });

         _IRouter.AddInterface(VCAddIS);
      }
      #endregion

      #region IViewModel
      public override void ModelUpdated()
      {

      }
      #endregion
   }
}
