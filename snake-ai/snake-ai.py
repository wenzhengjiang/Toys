#!/usr/bin/env python2
# coding=utf-8
# Contributor:
#    Wenzheng Jiang          <jwzh.hi@gmail.com>

import sys
import gtk
import cairo
import random
import glib

WIDTH = 15
HEIGHT = 15
DOT_SIZE = 10
# 27 * 30
ALL_DOTS = WIDTH * HEIGHT
RAND_POS = WIDTH-1
DELAY = 25 

UP = -WIDTH
DOWN = WIDTH 
LEFT = -1
RIGHT = 1
NONE = 0
INF = ALL_DOTS * 10 

move = [UP,DOWN,LEFT,RIGHT,NONE]

def xy(n):
    return n%WIDTH,n/WIDTH

class Board(gtk.DrawingArea):
    
    def __init__(self):
        super(Board,self).__init__()

        self.modify_bg(gtk.STATE_NORMAL,gtk.gdk.Color(0,0,0))
        self.set_size_request(WIDTH*DOT_SIZE,HEIGHT*DOT_SIZE)
        self.connect("expose_event",self.expose)
        self.first = True 
        self.init_game()
    
    def init_game(self):

        self.inGame = True
        self.dots = 3
        self.snake = [0] * ALL_DOTS
        self.apple = 10 * WIDTH + 10

        for i in xrange(self.dots):
            x = 5 - i 
            y = 5
            self.snake[i] = x + y * WIDTH
        self.locate_apple()
        if self.first:
            glib.timeout_add(DELAY,self.on_timer)
        self.pause = False
        self.debug = False
    def locate_apple(self):
        self.apple = self.snake[0]
        while self.apple in self.snake:
            x = random.randint(0,RAND_POS)
            y = random.randint(0,RAND_POS)
            self.apple = x + y * WIDTH
#        self.apple = 26 * WIDTH + 26
    def on_timer(self):
        if self.inGame:
            self.check_apple()
            self.check_collision()
            if not self.pause: 
                self.move()
            self.queue_draw()
            return True
        else:
            return False

    def expose(self,widget,event):
        cr = widget.window.cairo_create()

        if self.inGame:
            cr.set_source_rgb(0,0,0)
            cr.paint()

            cr.set_source_rgb(0.9, 0.1, 0.1);
            cr.rectangle(self.apple%WIDTH*DOT_SIZE,self.apple/WIDTH*DOT_SIZE,DOT_SIZE,DOT_SIZE)
            cr.fill()
            
            for k in range(1,self.dots):
                cr.set_source_rgb(0.4, 0.9, 0.4);
                cr.rectangle(self.snake[k]%WIDTH*DOT_SIZE,self.snake[k]/WIDTH*DOT_SIZE,DOT_SIZE,DOT_SIZE)
                cr.fill()
            cr.set_source_rgb(0.9, 0.1, 0.1);
            cr.rectangle(self.snake[0]%WIDTH*DOT_SIZE,self.snake[0]/WIDTH*DOT_SIZE,DOT_SIZE,DOT_SIZE)
            cr.fill()


        else:
            self.game_over(cr)

    def game_over(self,cr):
        w = self.allocation.width / 2
        h = self.allocation.height / 2

        (x,y,width,height,dx,dy) = cr.text_extents("Game Over")
        cr.set_source_rgb(65535,65535,65535)
        cr.move_to(w - width/2,h)
        cr.show_text("Game Over " + repr(self.dots))
        self.inGame = False

    def check_apple(self):
        if self.apple == self.snake[0]:
            self.dots = self.dots + 1
            self.locate_apple()

    def move(self):
        k = self.dots 
        self.snake[0] = self.get_next()
        head = self.snake[0];
        print xy(head)
        while k > 0:
            self.snake[k] = self.snake[k-1]
            k = k - 1
        for i in xrange(4):
            if self.isvalid(head,move[i]):
                print self.dis[head+move[i]],
            else:
                print INF,
        print '\n'
        if self.debug :
            self.pause = True
    def dfs(self,pos):
        self.vis[pos] = True
        self.dot_cnt += 1
        for i in xrange(4):
            if self.isvalid(pos,move[i]) and self.vis[pos+move[i]] == False:
                self.dfs(pos+move[i])

    def dead_end(self,pos):
        self.dot_cnt = 0
        self.vis = [False] * ALL_DOTS
        self.vis[pos] = True
        self.dfs(pos)
#        print self.dot_cnt,ALL_DOTS,self.dots-1
        if self.dot_cnt < ALL_DOTS - self.dots-10:
            return True
        return False

    def find_shortest_way(self):
        self.dis = [INF] * ALL_DOTS
        self.bfs(self.apple)
        mn = INF
        head = self.snake[0]
        idx = -1
        for i in xrange(4):
            t = -1
            if self.isvalid(head,move[i]) and ( True ) :
                if  self.dead_end(head+move[i]) == True:
                    #                    print "dead end",i
                    continue;
                t = self.dis[head+move[i]]
#                print i,t
                if t < mn:
                   idx = i
                   mn = t
                #                print "not valid",i
#        print "min dis is ",mn
        if idx == -1:
            return -1
        return self.snake[0]+move[idx]

    def find_longest_way(self,dest):
        self.dis = [0] * ALL_DOTS
        self.bfs(self.snake[dest])
        mx = 0
        head = self.snake[0]
        idx = -1
        for i in xrange(4):
           if self.isvalid(head,move[i]) and self.dis[head+move[i]] > mx:
               idx = i
               mx = self.dis[head+move[i]]
        if idx == -1: return -1 
        return self.snake[0]+move[idx]
    def find_rand_way(self):
        head = self.snake[0]
        idx = 0
        for i in xrange(4):
            if self.isvalid(head,move[i]):
                idx = i
                break
        return head + move[idx]
    def get_next(self):
        ret = self.find_shortest_way()
        t = self.dots - 1

        while ret == -1 and t > 0:
            ret = self.find_longest_way(t)
            t = t - 1
        if ret == -1:
            ret = self.find_rand_way()            
        return ret
    def bfs(self,start):
        queue = []
        queue.append(start)
        vis = [0] * ALL_DOTS
        vis[start] = 1
        flag = False
        self.dis[start] = 0
        while len(queue) != 0:
            cur = queue.pop(0)
            if cur == self.snake[0]: Flag = True
            for i in xrange(4) :
                if self.isvalid(cur,move[i]) and vis[cur+move[i]] == 0:
                    self.dis[cur+move[i]] = self.dis[cur] + 1
                    queue.append(cur+move[i])
                    vis[cur+move[i]] = 1    
        return flag

    def isvalid(self,cur,mv):
        x = cur % WIDTH
        y = cur / WIDTH
#        return False
        if mv == UP and y < 1: return False 
        if mv == DOWN and y > HEIGHT-2: return False
        if mv == LEFT and x < 1: return False
        if mv == RIGHT and x > WIDTH-2: return False
        if cur+mv in self.snake[:self.dots]: return False
        return True 

    def check_collision(self):
        k = self.dots
        while k > 0:
            if k > 4 and self.snake[0] == self.snake[k]:
                self.inGame = False
            k = k - 1

    def on_key_down(self,event):
        key = event.keyval
#        print key
        if key == 112:
            self.pause = not self.pause
        elif key == 113:
            self.inGame = False
        elif key == 114:
            self.init_game()
        elif key == 100:
            self.debug = not self.debug
class Snake(gtk.Window):

    def __init__(self):
        super(Snake,self).__init__()

        self.set_title('Snake')
        self.set_size_request(WIDTH*DOT_SIZE,HEIGHT*DOT_SIZE)
        self.set_resizable(False)
        self.set_position(gtk.WIN_POS_CENTER)

        self.board = Board()
        self.connect("key-press-event",self.on_key_down)
        self.add(self.board)

        self.connect("destroy",gtk.main_quit)
        self.show_all()

    def on_key_down(self,widget,event):

        key = event.keyval
        self.board.on_key_down(event)

Snake()
gtk.main()
