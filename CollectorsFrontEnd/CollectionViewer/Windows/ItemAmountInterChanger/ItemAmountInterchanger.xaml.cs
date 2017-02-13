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

namespace CollectorsFrontEnd
{
    /// <summary>
    /// Interaction logic for ItemAmountInterchanger.xaml
    /// </summary>
    public partial class ItemAmountInterchanger : UserControl
    {
        public string ActiveCardLong { get; set; }
        public string ActiveCollection { get; set; }
        public string CardName;
        public List<AmountInterchanger> ListInterchangers { get; set; }

        public ItemAmountInterchanger()
        {
            InitializeComponent();
        }

        public void SetCard(string aszCollection, string aszCardLong, List<CollectionViewGeneralization> alstGeneralizations)
        {
            CObjectListDisplay oParse = new CObjectListDisplay();
            oParse.SetCard(aszCardLong, null);
            CardName = oParse.CardName;
            LblCardName.Content = oParse.CardName;
            SPCollections.ItemsSource = ListInterchangers;
            ActiveCardLong = aszCardLong;
            ActiveCollection = aszCollection;
            PopulateColCounts(alstGeneralizations);

        }

        public void PopulateColCounts(List<CollectionViewGeneralization> alstGeneralizations)
        {
            // Add code here to fetch generalization amounts. Until then...
            foreach(var oGen in alstGeneralizations)
            {
                // Search through the generalizations for the desired card and grab the count.
                List<Tuple<string, string>> lstMetaTags = new List<Tuple<string, string>>();
                int iCount = 0;
                foreach(CObjectListDisplay oCOD in oGen.Items)
                {
                    if (oCOD.CardName == CardName)
                    {
                        if (MainWindow.SCI.IsSameIdentity(ActiveCardLong, oCOD.CardStringLong))
                        {
                            iCount = oCOD.iCount;
                            lstMetaTags = oCOD.MetaTags;
                        }
                    }
                }

                AmountInterchanger AI = new AmountInterchanger();
                AI.LblCollectionName.Content = oGen.Name;
                AI.MetaTags = lstMetaTags;
                AI.SetStartCount(iCount);

                ListInterchangers.Add(AI);
                SPCollections.Items.Refresh();
            }
        }
    }
}
