﻿using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Input;

namespace StoreFrontPro.Views
{
   public class RelayCommand : ICommand
   {
      private Action<object> execute;
      private Func<object, bool> canExecute;

      public event EventHandler CanExecuteChanged
      {
         add { CommandManager.RequerySuggested += value; }
         remove { CommandManager.RequerySuggested -= value; }
      }

      public RelayCommand(Action<object> execute, Func<object, bool> canExecute = null)
      {
         this.execute = execute;
         this.canExecute = canExecute;
      }

      public bool CanExecute(object parameter)
      {
         return this.canExecute == null || this.canExecute(parameter);
      }

      public void Execute(object parameter)
      {
         this.execute(parameter);
      }
   }

   abstract class ViewModel<T> : INotifyPropertyChanged, IViewModel where T: IModel
   {
      public event PropertyChangedEventHandler PropertyChanged;
      protected virtual void OnPropertyChanged([CallerMemberName] string propertyName = null)
      {
         PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
      }

      abstract public void ModelUpdated();

      // This MAY be exposed via a prettier property with the appropriate type in child classes.
      public T Model { get; protected set; }
      public string RoutingName { get; protected set; }

      public ViewModel(T Model, string RoutingName)
      {
         this.Model = Model;
         this.RoutingName = RoutingName;
      }

      public virtual void FreeModel()
      {
         Model.UnRegister(this);
      }
   }
}
