# Copyright (c) 2015,2016 CSIRO
# Australia Telescope National Facility (ATNF)
# Commonwealth Scientific and Industrial Research Organisation (CSIRO)
# PO Box 76, Epping NSW 1710, Australia
# atnf-enquiries@csiro.au
#
# This file is part of the ASKAP software distribution.
#
# The ASKAP software distribution is free software: you can redistribute it
# and/or modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of the License
# or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA.
#
"""
========================
Module :mod:`askap.jira`
========================

Access the (CSIRO) JIRA API

:author: Malte Marquarding <Malte.Marquarding@csiro.au>
"""
import os
import getpass
import requests

__all__ = ["Jira"]


class Jira(object):
    """Create an API request to JIRA.

    Example::

        from askap.jira import Jira

        jira = Jira()
        jira.authenticate()
        #jira.create_issue("SB1234", "A new issue", project="ASKAPTOS")
        jira.add_attachment("ASKAPTOS-3663", "/Users/mar637/temps.png")
        jira.add_comment("ASKAPTOS-3663", "test comment from API")

    """
    def __init__(self):
        self.user = os.getenv("JIRA_USER")
        "The JIRA user name (default unix username)"
        if self.user is None:
            self.user = getpass.getuser()
        self.server = "https://jira.csiro.au"
        "The JIRA server"
        self.rest = "/rest/api/2/"
        "The location of the REST API"
        self._session = requests.session()

    def authenticate(self):
        """
        Authenticate the JIRA session.
        """
        password = os.getenv("JIRA_PASSWORD")
        user = self.user
        if password is None:
            print("Please provide your JIRA (nexus) credentials")
            prompt = "Username (default '{}'):".format(user)
            user = raw_input(prompt)
            user = user or self.user
            password = getpass.getpass()
        r = self._session.post(self.server, auth=(user, password))
        r.raise_for_status()

    def create_issue(self, sbid, description, project="ASKAPTOS"):
        """
        Create an issue on the JIRA server for the specified `project`

        :param sbid: The Scheduling Block ID to reference
        :param description: the issue description
        :param project: the JIRA project to sie
        :return: the issue name (e.g. ASKAPTOS-1234)
        """
        url = "".join((self.server, self.rest, "issue"))
        data = {
            "fields": {
                "project": {"key": project},
                "summary": "SB{} annotations".format(sbid),
                "description": description,
                "issuetype": {"name": "Task"}
            }
        }
        r = self._session.post(url, json=data)
        r.raise_for_status()
        name = r.json()["key"]
        return name

    def add_attachment(self, name,  attachments):
        """
        Add an attachment to the specified issue

        :param name: the issue name
        :param attachments: the filename(s) of the attachment(s)
        """
        url = "".join((self.server, self.rest, "issue/", name,
                       "/attachments"))
        header = {"X-Atlassian-Token": "no-check"}
        files = []
        if isinstance(attachments, basestring):
            attachments = [attachments]
        for i, attachment in enumerate(attachments):
            handle = None
            if isinstance(attachment, basestring):
                fname = os.path.expanduser(os.path.expandvars(attachment))
                handle = open(fname, "rb")
            else:
                handle = attachment
            files.append(("file", handle))
        r = self._session.post(url, files=files, headers=header)
        r.raise_for_status()

    def add_comment(self, name, comment):
        """
        Add a comment to the specified issue

        :param name: the issue name
        :param comment: the comment string
        """
        url = "".join((self.server, self.rest, "issue/", name,
                       "/comment"))
        data = {"body": comment}
        r = self._session.post(url, json=data)
        r.raise_for_status()

    def link_issue(self, name, sbid):
        """
        Link the given issue to the Scheduling Block

        :param name: the issue name
        :param sbid: the Scheduling Block id
        """
        self.add_comment(name, "Added SB{}".format(sbid))


if __name__ == "__main__":
    jira = Jira()
    jira.authenticate()
    jira.add_attachment("ASKAPTOS-3663", "/Users/mar637/temps.png")
#    jira.add_comment("ASKAPTOS-3663", "test comment from API")
#    jira.link_issue("ASKAPTOS-3663", "0000")
