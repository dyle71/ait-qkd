/*
 * main_window.h
 * 
 * declares the main window for QKD module-manager
 * 
 * Autor: Oliver Maurhart, <oliver.maurhart@ait.ac.at>
 *
 * Copyright (C) 2013-2015 AIT Austrian Institute of Technology
 * AIT Austrian Institute of Technology GmbH
 * Donau-City-Strasse 1 | 1220 Vienna | Austria
 * http://www.ait.ac.at
 *
 * This file is part of the AIT QKD Software Suite.
 *
 * The AIT QKD Software Suite is free software: you can redistribute 
 * it and/or modify it under the terms of the GNU General Public License 
 * as published by the Free Software Foundation, either version 3 of 
 * the License, or (at your option) any later version.
 * 
 * The AIT QKD Software Suite is distributed in the hope that it will 
 * be useful, but WITHOUT ANY WARRANTY; without even the implied warranty 
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with the AIT QKD Software Suite. 
 * If not, see <http://www.gnu.org/licenses/>.
 */

 
#ifndef __QKD_QKD_MODULE_MANAGER_MAIN_WINDOW_H_
#define __QKD_QKD_MODULE_MANAGER_MAIN_WINDOW_H_


// ------------------------------------------------------------
// incs

// Qt
#include <QtGui/QMainWindow>


// ------------------------------------------------------------
// decl


namespace qkd {
    
namespace module_manager {

    
// fwd. decl.
class main_widget;

    
/**
 * This class represenst the main window of QKD Module Manager
 */
class main_window : public QMainWindow {
    

    Q_OBJECT
    
    
public:


    /**
     * ctor
     */
    main_window();
    
    
    /**
     * dtor
     */
    virtual ~main_window() {};
    
    
    /**
     * load window settings
     */
    void load_settings();
    
    
    /**
     * save window settings
     */
    void save_settings() const;
    
    
protected:


    /**
     * handle close event
     *
     * @param   cEvent      the event passed
     */
    virtual void closeEvent(QCloseEvent* cEvent);
    
    

protected slots:
    
    
    /**
     * quit app
     */
    virtual void quitApp();
    
    
private:
    
    
    /**
     * centers the window on the desktop with default width and height
     */
    void center_window();
    
    
    /**
     * the main widget 
     */
    main_widget * m_cMainWidget;    
    
};


}
}

#endif
