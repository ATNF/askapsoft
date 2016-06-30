from nose.tools import assert_equals, assert_items_equal
import askap.iceutils.monitoringprovider


class TestBuffer(object):
    def __init__(self):
        self.buffer = askap.iceutils.monitoringprovider.MonitoringBuffer()

    def test_add_points(self):
        points = {'a': 1, 'b': None}
        self.buffer.add_points(points, 1)
        assert_equals(len(self.buffer.get(['c'])), 0)
        assert_equals(len(self.buffer.get([])), 0)
        out = [dict(timestamp=1, name='a', value=1),
               dict(timestamp=1, name='b', value=None)]
        assert_items_equal(self.buffer.get(points.keys()), out)

    def test_add(self):
        for k,v in (('a', 1), ('b', None)):
            point = {'name': k, 'value': v, 'timestamp': 1}
            self.buffer.add(**point)
        out = [dict(status='OK', timestamp=1, value=1, name='a', unit=''),
               dict(status='OK', timestamp=1, value=None, name='b', unit='')]
        assert_items_equal(self.buffer.get(['a', 'b']), out)
