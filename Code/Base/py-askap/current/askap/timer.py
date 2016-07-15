import time

class Timer:
    def __init__(self):
        self.starttime = None
        self.endtime = None

    def start(self):
        self.starttime = time.time()

    def stop(self):
        self.endtime = time.time()

    def sec(self):
        if (self.starttime==None or self.endtime==None):
            return None
        return (self.endtime-self.starttime)

    def msec(self):
        sec = self.sec()
        if (sec == None):
            return None
        else:
            return sec*1000

    def elapsedmsec(self, str):
        msec = self.msec()
        if (msec==None):
            print 'Elapsed time for "{0}" undefined'.format(str)
        else:
            print '{0} took {1:.1f} msec'.format(str, msec)

    def elapsed(self, str):
        sec = self.sec()
        if (sec==None):
            print 'Elapsed time for "{0}" undefined'.format(str)
        else:
            print '{0} took {1:.1f} sec'.format(str, sec)
