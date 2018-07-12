/*
This program by Peter Semiletov <peter.semiletov@gmail.com> is public domain
*/

#include <QProcess>
#include <QDebug>
#include <QDesktopWidget>
#include <QApplication>
#include <QPixmap>
#include <QDir>
#include <QLibraryInfo>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QFontDialog>
#include <QFont>
#include <QDateTime>
#include <QPainter>

#include "mainwindow.h"


bool file_exists (const QString &fileName)
{
  if (fileName.isEmpty())
     return false;

  return QFile::exists (fileName);
}


QHash <QString, QString> hash_load_keyval (const QString &data)
{
  QHash <QString, QString> result;
  
  QStringList l = data.split ("\n");

  foreach (QString s, l)
          {
           QStringList sl = s.split (":");
           if (sl.size() > 1)
               result.insert (sl[0], sl[1]);
          }

  return result;
}


QString hash_get_val (QHash<QString, QString> &h,
                      const QString &key,
                      const QString &def_val)
{
  QString result = h.value (key);
  if (result.isNull() || result.isEmpty())
     {
      result = def_val;
      h.insert (key, def_val);
     }

  return result;
}


QString qstring_load (const QString &fileName, const char *enc)
{
  QFile file (fileName);

  if (! file.open (QFile::ReadOnly | QFile::Text))
     return QString();

  QTextStream in (&file);
  in.setCodec (enc);

  return in.readAll();
}


void MainWindow::show_at_center()
{
  QDesktopWidget *desktop = QApplication::desktop();
  
  int x = (desktop->width() - size().width()) / 2;
  int y = (desktop->height() - size().height()) / 2;
  y -= 50;
 
  move (x, y);
}


MainWindow::MainWindow (QWidget *parent): QMainWindow (parent)
{
  qtTranslator.load (QString ("qt_%1").arg (QLocale::system().name()),
                     QLibraryInfo::location (QLibraryInfo::TranslationsPath));
       
  qApp->installTranslator (&qtTranslator);

  myappTranslator.load (":/translations/upsm_" + QLocale::system().name());
  qApp->installTranslator (&myappTranslator);
    
  QDir dr;
  s_config_fname = dr.homePath() + "/.config/upsm.conf";
  settings = new QSettings (s_config_fname, QSettings::IniFormat);

  command = settings->value ("command", "upsc serverups@localhost").toString();
  polltime = settings->value ("polltime", "5000").toInt();
  logsize = settings->value ("logsize", "1024").toInt();
  
  paint_rect = new QImage (64, 64, QImage::Format_ARGB32);
/*
  QPixmap pxm_red (64, 64);
  pxm_red.fill (Qt::red);
  icon_red.addPixmap (pxm_red);

  QPixmap pxm_green (64, 64);
  pxm_green.fill (Qt::green);
  icon_green.addPixmap (pxm_green);

  QPixmap pxm_yellow (64, 64);
  pxm_yellow.fill (Qt::yellow);
  icon_yellow.addPixmap (pxm_yellow);
*/
  //tray_icon.setIcon (style.standardIcon(QStyle::SP_MessageBoxQuestion));
//  tray_icon.setIcon (icon_green);

  tray_icon.setVisible (true);


  resize (800, 400);
  show_at_center();
  
  setCentralWidget (&main_widget);
  
  main_widget.addTab (&editor, tr ("Stats"));
  
  editor.setFont (QFont (settings->value("font_name", "Serif").toString(), 
                         settings->value("font_size", "24").toInt()));

  editor.setReadOnly (true);
  
  connect(&tray_icon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
          this, SLOT(iconActivated(QSystemTrayIcon::ActivationReason)));

  main_widget.addTab (&log, tr ("Log"));
  connect (&main_widget, SIGNAL(currentChanged(int)), this, SLOT(pageChanged(int)));

  
 
  QWidget *settings_widget = new QWidget; 
  
  QVBoxLayout *la_settings = new QVBoxLayout; 
  settings_widget->setLayout (la_settings);
  
  QHBoxLayout *la_command = new QHBoxLayout; 
  QLabel *l_command = new QLabel (tr ("Command"));
  led_command = new QLineEdit;
  la_command->addWidget (l_command);  
  la_command->addWidget (led_command);  
  led_command->setText (settings->value ("command", "upsc serverups@localhost").toString());
  la_settings->addLayout (la_command);
  

  QHBoxLayout *la_polltime = new QHBoxLayout; 
  QLabel *l_polltime = new QLabel (tr ("Polling time, msecs"));
  led_polltime = new QLineEdit;
  la_polltime->addWidget (l_polltime);  
  la_polltime->addWidget (led_polltime);  
  led_polltime->setText (settings->value ("polltime", "5000").toString());
  la_settings->addLayout (la_polltime);

  QHBoxLayout *la_logsize = new QHBoxLayout; 
  QLabel *l_logsize = new QLabel (tr ("Log size, in records"));
  led_logsize = new QLineEdit;
  la_logsize->addWidget (l_logsize);  
  la_logsize->addWidget (led_logsize);  
  led_logsize->setText (settings->value ("logsize", "1024").toString());
  la_settings->addLayout (la_logsize);

  cb_hide_from_taskbar = new QCheckBox (tr ("Hide from taskbar"));
  cb_hide_from_taskbar->setTristate (false);

  if (settings->value ("hide_from_taskbar", "0").toBool())
      cb_hide_from_taskbar->setChecked (true);

  la_settings->addWidget (cb_hide_from_taskbar);

  cb_run_minimized = new QCheckBox (tr ("Run minimized"));
  cb_run_minimized->setTristate (false);

  if (settings->value ("run_minimized", "0").toBool())
     {
      cb_run_minimized->setChecked (true);
      hide();
     } 
      
  la_settings->addWidget (cb_run_minimized);


  QPushButton *bt_select_font = new QPushButton (tr ("Select font")); 
  connect (bt_select_font, SIGNAL (clicked()),this, SLOT (bt_select_font_clicked()));
  la_settings->addWidget (bt_select_font);

  la_settings->addStretch (0);

  QPushButton *bt_saveapply = new QPushButton (tr ("Apply and save")); 
  connect (bt_saveapply, SIGNAL (clicked()),this, SLOT (bt_apply_clicked()));
  la_settings->addWidget (bt_saveapply);

  
  main_widget.addTab (settings_widget, tr ("Settings"));
  
  QPlainTextEdit *help_widget = new QPlainTextEdit;
  
  
  QString loc = QLocale::system().name().left (2);

  QString filename (":/manuals/");
  filename.append (loc);
  
  if (! file_exists (filename))
      filename = ":/manuals/en";
  
  help_widget->setPlainText (qstring_load (filename, "UTF-8"));
  
  main_widget.addTab (help_widget, tr ("Help"));
  
  
  timer = new QTimer(this);
  connect (timer, SIGNAL(timeout()), this, SLOT(update_stats()));
  
  update_stats();
  
  timer->start (polltime);
   
  if (settings->value ("hide_from_taskbar", "0").toBool())
      {
       setWindowFlags (Qt::Tool);  
       setAttribute (Qt::WA_QuitOnClose);
      } 
  else
      setWindowFlags (Qt::Window);  
}


void MainWindow::bt_apply_clicked()
{
  settings->setValue ("command", led_command->text());
  settings->setValue ("polltime", led_polltime->text());
  settings->setValue ("logsize", led_logsize->text());
  
  command = settings->value ("command", "no").toString();
  polltime = settings->value ("polltime", "5000").toInt();
  logsize = settings->value ("logsize", "1024").toInt();

  settings->setValue ("run_minimized", cb_run_minimized->isChecked());
  settings->setValue ("hide_from_taskbar", cb_hide_from_taskbar->isChecked());
  
  if (settings->value ("hide_from_taskbar", "0").toBool())
      {
       setWindowFlags (Qt::Tool);  
       setAttribute (Qt::WA_QuitOnClose);
      } 
  else
      setWindowFlags (Qt::Window);  
  
  timer->stop();
  timer->start (polltime);
}
    

void MainWindow::update_stats()
{
  if (command.isEmpty())
     return;

  QProcess procmon;
  procmon.start (command);
    
  if (! procmon.waitForStarted())
      return;

  if (! procmon.waitForFinished())
     return;
  
  
  QByteArray result = procmon.readAll();
  QString data = result.data();
    
  QHash <QString, QString> h = hash_load_keyval (data);
                      
  QString out;                      

  QString input_v = hash_get_val (h, "input.voltage","NOOO");
  QString output_v = hash_get_val (h, "output.voltage","NOOO");

  out.append (tr ("Input: ")); 
  out.append (input_v);
  out.append ("\n");

  out.append (tr ("Output: ")); 
  out.append (output_v);
  out.append ("\n");
  
  out.append (tr ("Load: ")); 
  out.append ( hash_get_val (h, "ups.load","NOOO"));
  out.append ("\n");
    
  QString status = hash_get_val (h, "ups.status","NOOO").trimmed();
  
  //"OL" "OL TRIM" "OB"
  
  QString t;
    
  if (status == "OL")
     {
      t = tr ("voltage normal");
      paint_rect->fill (Qt::darkGreen);
      //tray_icon.setIcon (style.standardIcon(QStyle::SP_MessageBoxInformation));
      //tray_icon.setIcon (icon_green);
//      QPixmap pm = QPixmap::fromImage (*paint_rect);
  //    tray_icon.setIcon (QIcon (pm));
     }
  else    
  if (status == "OL TRIM")
     {
      t = tr ("ups is trimming voltage");
      paint_rect->fill (Qt::darkRed);
  //    tray_icon.setIcon (icon_yellow);
     } 
  else
  if (status == "OB")
     {
      t = tr ("battery mode");
      paint_rect->fill (Qt::red);
//      tray_icon.setIcon (icon_red);
     } 
  
  out.append (t);
  out.append ("\n");
  
  editor.setPlainText (out);
  
  if (slst_log.size() > logsize)
     slst_log.removeLast();
  
  QDateTime dt (QDateTime::currentDateTime());
       
  slst_log.prepend (out);   
  
  slst_log.prepend ("---------" + dt.toString ("hh:mm:ss") + "---------");

  QPainter pnt (paint_rect);
  QFont fnt ("Sans");
  fnt.setPixelSize (26);  
     
  pnt.setFont (fnt);
  pnt.setPen (Qt::cyan);
  pnt.drawText(QRect (1, 1, 64, 32), Qt::AlignLeft, input_v.left (input_v.indexOf (".")));

  pnt.setPen (Qt::white);
  pnt.drawText(QRect (1, 32, 64, 32), Qt::AlignLeft, output_v.left (input_v.indexOf (".")));
  
  QPixmap pm = QPixmap::fromImage (*paint_rect);
  tray_icon.setIcon (QIcon (pm));
}


void MainWindow::iconActivated(QSystemTrayIcon::ActivationReason reason)
{
  switch (reason) 
         {
          case QSystemTrayIcon::Trigger:
         // case QSystemTrayIcon::DoubleClick:
                  
          if (! isVisible())        
             {     
              show_at_center();                            
              show();
              activateWindow();
              raise();
             }
          else
              hide(); 
                            
          break;
    
          default:
                  ;
         }
}


void MainWindow::closeEvent (QCloseEvent *event)
{
  log.disconnect();
  event->accept();
}    


MainWindow::~MainWindow()
{
  delete paint_rect;
  delete settings;  
}


void MainWindow::bt_select_font_clicked()
{
  bool ok;
  QFont font = QFontDialog::getFont (&ok, QFont (settings->value("font_name", "Serif").toString(),
                                                 settings->value("font_size", "24").toInt()), this);
  if (ok) 
     {
      settings->setValue ("font_name", font.family());
      settings->setValue ("font_size", font.pointSize());
       
      editor.setFont (QFont (settings->value("font_name", "Serif").toString(), 
                             settings->value("font_size", "24").toInt()));
     }  
}


void MainWindow::pageChanged (int index)
{
  if (main_widget.count() == 4)
     if (index == 1)
        log.setPlainText (slst_log.join ("\n"));
}
