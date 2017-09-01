﻿using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Net;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Media.Imaging;

namespace StoreFrontPro.Server
{
   partial class ServerInterface
   {
      public class CardIFace
      {
         public string SZ_IMAGE_CACHE_PATH = "";

         private class ImageDownloadedEventArgs : EventArgs
         {
            public CardModel DataModel;
            public EventArgs e;

            public ImageDownloadedEventArgs(CardModel aDataModel, EventArgs ae)
            {
               DataModel = aDataModel;
               e = ae;
            }
         }

         public string GetProtoType(string szCardName)
         {
            return SCI.GetCardPrototype(szCardName);
         }

         public void DownloadAndCacheImage(Action<BitmapImage> aCallback, CardModel aoCardModel)
         {
            Thread downloadAndLoadImageThread = new Thread(() => { inDownloadAndCacheImage(aCallback, aoCardModel); });
            downloadAndLoadImageThread.IsBackground = true;
            downloadAndLoadImageThread.Start();
         }

         private void inDownloadAndCacheImage(Action<BitmapImage> aCallback, CardModel aoCardModel)
         {
            //Download the image.
            string szMUID = aoCardModel.GetAttr("multiverseid");
            string szSet = aoCardModel.GetAttr("set");
            if (SZ_IMAGE_CACHE_PATH == "")
            {
               SZ_IMAGE_CACHE_PATH = SCI.GetImagesPath();
            }
            string szBasePath = SZ_IMAGE_CACHE_PATH + "/_" + szSet + "/";
            string szFilePath = szBasePath + aoCardModel.CardName + ".jpg";

            // Check if we already have the image
            string szDirectoryName = Path.GetDirectoryName(szFilePath);
            if (!Directory.Exists(szDirectoryName))
            {
               Directory.CreateDirectory(szDirectoryName);
            }

            // Less than 100 bytes, ignore the file.
            if (!File.Exists(szFilePath) || (new System.IO.FileInfo(szFilePath).Length < 100))
            {
               if (File.Exists(szFilePath))
               {
                  File.Delete(szFilePath);
               }

               using (WebClient client = new WebClient())
               {
                  string szURL;
                  if (!string.IsNullOrEmpty(szMUID))
                  {
                     szURL = @"http://gatherer.wizards.com/Handlers/Image.ashx?multiverseid=" +
                             szMUID + @"&type=card";
                  }
                  else
                  {
                     szURL = @"http://gatherer.wizards.com/Handlers/Image.ashx?name=" +
                             aoCardModel.CardName + "&type=card";
                  }

                  // Download synchronously.
                  try
                  {
                     client.DownloadFile(new Uri(szURL, UriKind.RelativeOrAbsolute), szFilePath);
                  }
                  catch
                  {
                     // Connection issues, various things can fail here
                     if (File.Exists(szFilePath)) { File.Delete(szFilePath); }
                  }

               }
            }

            loadImageFromFile(szFilePath, aCallback);
         }

         private void loadImageFromFile(string aszFileName, Action<BitmapImage> aCallback)
         {
            if (!File.Exists(aszFileName)) { return; }

            try
            {
               BitmapImage bitmap = new BitmapImage();
               using (FileStream stream = File.OpenRead(aszFileName))
               {
                  bitmap.BeginInit();
                  bitmap.StreamSource = stream;
                  bitmap.CacheOption = BitmapCacheOption.OnLoad;
                  bitmap.EndInit();
                  bitmap.Freeze();
               }
               aCallback(bitmap);
            }
            catch
            {
               aCallback(null);
            }
         }
      }
   }
}
