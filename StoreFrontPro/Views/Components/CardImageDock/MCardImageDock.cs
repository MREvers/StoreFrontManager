﻿using StoreFrontPro.Server;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace StoreFrontPro.Views.Components.VCardImageDock
{
   class MCardImageDock : IModel
   {
      public CardModel ModelDisplay { get; private set; }

      public MCardImageDock()
      {

      }

      public void SetDisplayImage(CardModel oModel)
      {
         ModelDisplay = oModel;
         NotifyViewModel();
      }

      #region IModel
      private List<IViewModel> m_lstViewers = new List<IViewModel>();
      public void Register(IViewModel item)
      {
         m_lstViewers.Add(item);
      }

      public void UnRegister(IViewModel item)
      {
         m_lstViewers.Remove(item);
      }

      public void NotifyViewModel()
      {
         m_lstViewers.ForEach(x => x.ModelUpdated());
      }

      public void Sync(bool ASync = true)
      {

      }

      public void EnableNotification(bool abNotify)
      {
         throw new NotImplementedException();
      }

      public void DisableNotification()
      {
         throw new NotImplementedException();
      }
      #endregion
   }
}