﻿using StoreFrontPro.Server;
using StoreFrontPro.Views.Components.CopySelector;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace StoreFrontPro.Views.Interfaces.CardInterface
{
   class VMCardChanger : ViewModel<CardModel>, IVCISupporter, IViewComponent
   {
      #region Bindings
      public ObservableCollection<VAttributeEditorItem> IdentifyingAttributes { get; set; }
         = new ObservableCollection<VAttributeEditorItem>();

      private VMCopySelector _CopySelector
      {
         get { return (VMCopySelector) CopySelector.DataContext; }
      }
      public VCopySelector CopySelector { get; set; }

      public RelayCommand SaveCommand { get; set; }
      public RelayCommand CancelCommand { get; set; }
      #endregion

      #region Public Methods
      public VMCardChanger(CardModel Model, string RoutingName)
         : base(Model, RoutingName)
      {
         SaveCommand = new RelayCommand(eSaveChanges);
         CancelCommand = new RelayCommand(eCancelChanges);

         ViewClass copySelector = ViewFactory.CreateCopySelector(Model, RoutingName);
         CopySelector = (VCopySelector) copySelector.View;
      }

      public override void ModelUpdated()
      {
         throw new NotImplementedException();
      }

      public List<string> GetEdittingUIDs()
      {
         return _CopySelector.GetSelectedCopyList().Select(x => x.Specifier).ToList();
      }

      public void SetEditting(CardModel Model)
      {
         bool bPersistSelection = true;
         if( (this.Model != null) && 
             (this.Model.PrototypeName != Model.PrototypeName) )
         {
            // Undo any preview changes.
            this.Model.Sync(false);
            bPersistSelection = false;
         }

         this.Model = Model;
         _CopySelector.SetEditting(Model, bPersistSelection);
         setIdentifyingTraitViews();
      }
      #endregion

      #region Private Methods

      private void setIdentifyingTraitViews()
      {
         clearIdentifyingTraits();

         CardModelBase oProto = CardModel.GetPrototype(Model.PrototypeName);
         foreach( var trait in oProto.AttributeOptions )
         {
            addIdentifyingTrait(trait);
         }
      }

      private void addIdentifyingTrait(KeyValuePair<string,List<string>> trait)
      {
         ViewClass traitView = ViewFactory.CreateAttributeEditorItem(trait.Key, trait.Value, trait.Key);
         IdentifyingAttributes.Add((VAttributeEditorItem) traitView.View);

         // Set the trait to the current one.
         string szCurrentTrait = Model.GetAttr(trait.Key);
         ((VMAttributeEditorItem) traitView.ViewModel).Set = szCurrentTrait;
         ((VMAttributeEditorItem) traitView.ViewModel).DisplayEvent += DisplayEventHandler;
      }

      private void clearIdentifyingTraits()
      {
         foreach( var viewTrait in IdentifyingAttributes )
         {
            ((VMAttributeEditorItem) viewTrait.DataContext).DisplayEvent -= DisplayEventHandler;
         }
         IdentifyingAttributes.Clear();
      }

      private VMAttributeEditorItem editorItem(VAttributeEditorItem item)
      {
         return (VMAttributeEditorItem) item.DataContext;
      }
      #endregion

      #region Event Handlers
      /// <summary>
      /// This relies on the fact that the model represents
      /// a possible state of the card. i.e. paired traits are
      /// matched properly.
      /// </summary>
      private void eSaveChanges(object canExecute)
      {
         foreach( var copy in _CopySelector.GetSelectedCopyList() )
         {
            foreach(var trait in Model.IdentifyingAttributes)
            {
               Model.SetAttr(trait.Item1, trait.Item2, copy.Specifier);
            }
         }
         
         // Notify the that the server has been contacted.
         fireSave();
      }

      private void eCancelChanges(object canExecute)
      {
         this.Model.Sync(false);
         firePreview();
      }

      /// <summary>
      /// Expects aszRouting to be the attribute name.
      /// </summary>
      /// <param name="aszRouting"></param>
      /// <param name="aszNewVal"></param>
      private void eIdentifierChanged(string aszRouting, string aszNewVal)
      {
         CardModelBase oProto = CardModel.GetPrototype(Model.PrototypeName);

         // Find the index of the newly selected value.
         var lstOptions = oProto.AttributeOptions[aszRouting];
         int index = lstOptions.FindIndex(a => a == aszNewVal);
         
         // UPDATE THIS WHEN SELECTING UID. TODO
         Model.PreviewAttr(aszRouting, aszNewVal);

         // Update any paired traits
         foreach( var pairedTrait in ServerInterface.Card.GetPairedAttributes() )
         {
            string szSearch = "";
            if( pairedTrait.Item1 == aszRouting )
            {
               szSearch = pairedTrait.Item2;
            }
            else if ( pairedTrait.Item2 == aszRouting )
            {
               szSearch = pairedTrait.Item1;
            }

            if( szSearch != "" )
            {
               var otherPairItem = editorItem(
                  IdentifyingAttributes.FirstOrDefault
                  (x => editorItem(x).AttributeName == szSearch));

               if( otherPairItem != null )
               {
                  otherPairItem.Model.DisableNotification();
                  otherPairItem.Set = otherPairItem.Model.Options[index];
                  Model.PreviewAttr(otherPairItem.Model.Name, otherPairItem.Set);
                  otherPairItem.Model.EnableNotification();
               }
            }
         }

        firePreview();
      }


      #endregion

      #region Private Methods
      /// <summary>
      /// Tells the owner of this object that the model has a change,
      /// and that it should acknowledge those changes. e.g. by displaying
      /// a new image of the object.
      /// </summary>
      public void firePreview()
      {
         DisplayEventArgs eArgs = new DisplayEventArgs(VCICardChanger.PreviewChange);
         DisplayEvent?.Invoke(this, eArgs);
      }

      public void fireSave()
      {
         DisplayEventArgs eArgs = new DisplayEventArgs(VCICardChanger.SubmitChange);
         DisplayEvent?.Invoke(this, eArgs);
      }
      #endregion

      #region IViewComponent
      public event DisplayEventHandler DisplayEvent;

      public List<StoreFrontMenuItem> GetMenuItems()
      {
         throw new NotImplementedException();
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

         // TODO: the routing is important here because there is more than one.
         VCIAttributeEditorItem VCAddIS = new VCIAttributeEditorItem(
            SelectionChanged: (x) => { return (x as VMCardChanger).eIdentifierChanged; });

         _IRouter.AddInterface(VCAddIS);
      }
      #endregion
   }
}
