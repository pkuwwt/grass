"""
MODULE: gselect

CLASSES:
 * Select
 * TreeCrtlComboPopup

PURPOSE: Custon control that selects GRASS GIS elements

AUTHORS: The GRASS Development Team. Michael Barton & Martin Landa

COPYRIGHT: (C) 2007 by the GRASS Development Team
           This program is free software under the GNU General Public
           License (>=v2). Read the file COPYING that comes with GRASS
           for details.
"""

import os
import sys

import wx
import wx.combo

import globalvar
import gcmd
from preferences import globalSettings as UserSettings

class Select(wx.combo.ComboCtrl):
    def __init__(self, parent, id, size,
                 type, multiple=False, mapsets=None):
        """
        Custom control to create a ComboBox with a tree control
        to display and select GIS elements within acessible mapsets.
        Elements can be selected with mouse. Can allow multiple selections, when
        argument multiple=True. Multiple selections are separated by commas.
        """
        wx.combo.ComboCtrl.__init__(self, parent=parent, id=id, size=size)

        self.tcp = TreeCtrlComboPopup()
        self.SetPopupControl(self.tcp)
        self.SetPopupExtents(0,100)
        self.tcp.GetElementList(type, mapsets)
        self.tcp.SetMultiple(multiple)

    def SetElementList(self, type):
        self.tcp.seltree.DeleteAllItems()
        self.tcp.GetElementList(type)

class TreeCtrlComboPopup(wx.combo.ComboPopup):
    """
    Create a tree ComboBox for selecting maps and other GIS elements
    in accessible mapsets within the current location
    """

    # overridden ComboPopup methods
    def Init(self):
        self.value = [] # for multiple is False -> len(self.value) in [0,1]
        self.curitem = None
        self.multiple = False

    def Create(self, parent):
        self.seltree = wx.TreeCtrl(parent, style=wx.TR_HIDE_ROOT
                                   |wx.TR_HAS_BUTTONS
                                   |wx.TR_SINGLE
                                   |wx.TR_LINES_AT_ROOT
                                   |wx.SIMPLE_BORDER
                                   |wx.TR_FULL_ROW_HIGHLIGHT)
        self.seltree.Bind(wx.EVT_MOTION, self.OnMotion)
        self.seltree.Bind(wx.EVT_LEFT_DOWN, self.OnLeftDown)
        self.seltree.Bind(wx.EVT_TREE_ITEM_EXPANDING, self.mapsetExpanded)
        self.seltree.Bind(wx.EVT_TREE_ITEM_COLLAPSED, self.mapsetCollapsed)
        self.seltree.Bind(wx.EVT_TREE_ITEM_ACTIVATED, self.mapsetActivated)
        self.seltree.Bind(wx.EVT_TREE_SEL_CHANGED, self.mapsetSelected)

    # the following dummy handler are needed to keep tree events from propagating up to
    # the parent GIS Manager layer tree
    def mapsetExpanded(self, event):
        pass

    def mapsetCollapsed(self, event):
        pass

    def mapsetActivated(self, event):
        pass

    def mapsetSelected(self, event):
        pass
    # end of dummy events

    def GetControl(self):
        return self.seltree

    def GetStringValue(self):
        str = ""
        for value in self.value:
            str += self.seltree.GetItemText(value) + ","
        str = str.rstrip(',')
        return str

    def OnPopup(self):
        """Limited only for first selected"""
        if len(self.value) > 0:
            self.seltree.EnsureVisible(self.value[0])
            self.seltree.SelectItem(self.value[0])

    def SetStringValue(self, value):
        # this assumes that item strings are unique...
        root = self.seltree.GetRootItem()
        if not root:
            return
        found = self.FindItem(root, value)
        if found:
            self.value.append(found)
            self.seltree.SelectItem(found)

    def GetAdjustedSize(self, minWidth, prefHeight, maxHeight):
        return wx.Size(minWidth, min(200, maxHeight))

    def GetElementList(self, element, mapsets=None):
        """
        Get list of GIS elements in accessible mapsets and display as tree
        with all relevant elements displayed beneath each mapset branch
        """
        # get current mapset
        cmdlist = ['g.gisenv', 'get=MAPSET']
        curr_mapset = gcmd.Command(cmdlist).ReadStdOutput()[0]
        
        # list of mapsets in current location
        if mapsets is None:
            mapsets = UserSettings.Get(group='general', key='mapsetPath', subkey='value', internal=True)

        # map element types to g.mlist types
        elementdict = {'cell':'rast',
                       'raster':'rast',
                       'rast':'rast',
                       'raster files':'rast',
                       'grid3':'rast3d',
                       'rast3d':'rast3d',
                       'raster3D':'rast3d',
                       'raster3D files':'rast3d',
                       'vector':'vect',
                       'vect':'vect',
                       'binary vector files':'vect',
                       'dig':'oldvect',
                       'oldvect':'oldvect',
                       'old vector':'oldvect',
                       'dig_ascii':'asciivect',
                       'asciivect':'asciivect',
                       'asciivector':'asciivect',
                       'ascii vector files':'asciivect',
                       'icons':'icon',
                       'icon':'icon',
                       'paint icon files':'icon',
                       'paint/labels':'labels',
                       'labels':'labels',
                       'label':'labels',
                       'paint label files':'labels',
                       'site_lists':'sites',
                       'sites':'sites',
                       'site list':'sites',
                       'site list files':'sites',
                       'windows':'region',
                       'region':'region',
                       'region definition':'region',
                       'region definition files':'region',
                       'windows3d':'region3d',
                       'region3d':'region3d',
                       'region3D definition':'region3d',
                       'region3D definition files':'region3d',
                       'group':'group',
                       'imagery group':'group',
                       'imagery group files':'group',
                       '3d.view':'3dview',
                       '3dview':'3dview',
                       '3D viewing parameters':'3dview',
                       '3D view parameters':'3dview'}

        if element not in elementdict:
            self.AddItem(_('Not selectable element'))
            return

        # get directory tree nodes
        # reorder mapsets based on search path (TODO)
        for i in range(len(mapsets)):
            if i > 0 and mapsets[i] == curr_mapset:
                mapsets[i] = mapsets[0]
                mapsets[0] = curr_mapset

        for dir in mapsets:
            dir_node = self.AddItem('Mapset: '+dir)
            self.seltree.SetItemTextColour(dir_node,wx.Colour(50,50,200))
            try:
                cmdlist = ['g.mlist', 'type=%s' % elementdict[element], 'mapset=%s' % dir]
                elem_list = gcmd.Command(cmdlist).ReadStdOutput()
                elem_list.sort()
                for elem in elem_list:
                    if elem != '':
                        self.AddItem(elem+'@'+dir, parent=dir_node)
            except:
                continue

            if self.seltree.ItemHasChildren(dir_node):
                self.seltree.Expand(dir_node)

    # helpers
    def FindItem(self, parentItem, text):
        item, cookie = self.seltree.GetFirstChild(parentItem)
        while item:
            if self.seltree.GetItemText(item) == text:
                return item
            if self.seltree.ItemHasChildren(item):
                item = self.FindItem(item, text)
            item, cookie = self.seltree.GetNextChild(parentItem, cookie)
        return wx.TreeItemId();


    def AddItem(self, value, parent=None):
        if not parent:
            root = self.seltree.GetRootItem()
            if not root:
                root = self.seltree.AddRoot("<hidden root>")
            parent = root

        item = self.seltree.AppendItem(parent, text=value)
        return item

    def OnMotion(self, evt):
        # have the selection follow the mouse, like in a real combobox
        item, flags = self.seltree.HitTest(evt.GetPosition())
        if item and flags & wx.TREE_HITTEST_ONITEMLABEL:
            self.seltree.SelectItem(item)
            self.curitem = item
        evt.Skip()

    def OnLeftDown(self, evt):
        # do the combobox selection
        item, flags = self.seltree.HitTest(evt.GetPosition())
        if item and flags & wx.TREE_HITTEST_ONITEMLABEL:
            self.curitem = item

            if self.seltree.GetRootItem() == self.seltree.GetItemParent(item):
                self.value = [] # cannot select mapset item
            else:
                if self.multiple is True:
                    self.value.append(item)
                else:
                    self.value = [item, ]

            self.Dismiss()

        evt.Skip()

    def SetMultiple(self, value):
        """Select multiple items?"""
        self.multiple = value



