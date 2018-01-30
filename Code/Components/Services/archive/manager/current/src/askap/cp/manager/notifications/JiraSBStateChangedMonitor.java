/*
 * Copyright (c) 2016 CSIRO - Australia Telescope National Facility (ATNF)
 * 
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * PO Box 76, Epping NSW 1710, Australia
 * atnf-enquiries@csiro.au
 * 
 * This file is part of the ASKAP software distribution.
 * 
 * The ASKAP software distribution is free software: you can redistribute it
 * and/or modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 * 
 * @author Daniel Collins <daniel.collins@csiro.au>
 */
package askap.cp.manager.notifications;

import askap.cp.manager.svcclients.IFCMClient;
import askap.interfaces.schedblock.ObsState;
import askap.util.ParameterSet;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import org.apache.log4j.Logger;

/**
 * Implements Scheduling block state change notifications via JIRA.
 * 
 * @author Daniel Collins <daniel.collins@csiro.au>
 */
public final class JiraSBStateChangedMonitor extends SBStateMonitor {

    /**
     * Logger
     */
    private static final Logger logger = Logger.getLogger(JiraSBStateChangedMonitor.class.getName());

	/** 
	 * The configuration.
	 */
	private final ParameterSet itsConfig;

    /**
     * Facility Configuration Manager client wrapper instance
     */
    private final IFCMClient itsFCM;

	/**
	 * 
	 * @param config 	The configuration
	 * @param fcm		The FCM client
	 */
	public JiraSBStateChangedMonitor(ParameterSet config, IFCMClient fcm) {
		this.itsConfig = config;
		this.itsFCM = fcm;
	}

	/**
	 * Issues a scheduling block state changed notification.
	 * 
	 * @param sbid: The scheduling block ID.
	 * @param newState: The new scheduling block state.
	 * @param updateTime: The update timestamp string. 
	 * @throws askap.cp.manager.notifications.NotificationException 
	 */
	@Override
	public void notify(long sbid, ObsState newState, String updateTime) 
			throws NotificationException {
        try {
			logger.debug("creating ProcessBuilder");

			List<String> commandList = new ArrayList<String>();
			commandList.add("schedblock");
			commandList.add("annotate");
			commandList.add(Long.toString(sbid));
			commandList.add("--comment");
			commandList.add("\"Ready for data processing\"");

			// Fcm:
			// common.jira.issue
			// common.jira.project

			// TODO: disabling the JIRA project handling for now. 
			// The JIRA project is valid even if the issue ID is not specified
			/*
			final String jiraProject = itsConfig.getString("sbstatemonitor.jira.project", null);
			if (jiraProject != null) {
				logger.debug("Using JIRA project " + jiraProject);
				commandList.add("--project");
				commandList.add(jiraProject);
			}
			*/

			// We have 3 prioritised sources of the JIRA issue ID:
			// 1 - CP Manager parset, which overrides
			// 2 - FCM common.jira.issue, which overrides
			// 3 - schedblock annotate default behaviour

			// Try the parset first
			String jiraIssueId = itsConfig.getString("common.jira.issue", null);

			// If parset not found, try FCM
			if (jiraIssueId == null) {
				ParameterSet parset = itsFCM.get("common.jira.");
				if (!parset.isEmpty()) {
					jiraIssueId = parset.getString("issue", null);
					logger.debug("JIRA issue from FCM: " + jiraIssueId);
				}
			}

			// If we have an issue ID from parset or FCM, then append to the command,
			// otherwise do nothing and fall back on the schedblock CLI defaults.
			if (jiraIssueId != null) {
//				if (jiraProject == null) {
//					logger.warn("JIRA issue ID specified without a project specification");
//				} 
				logger.debug("Using JIRA issue ID " + jiraIssueId);
				commandList.add("--issue");
				commandList.add(jiraIssueId);
			} else {
				logger.warn("Issuing JIRA notification without a JIRA Issue ID");
			}

            ProcessBuilder pb = new ProcessBuilder(commandList);

			// ensure that JIRA authentication environment variables are set
			// TODO: should we abort in this case?
			if (!(pb.environment().containsKey("JIRA_USER") ||
				  pb.environment().containsKey("JIRA_PASSWORD"))) {
				logger.error("JIRA credentials not set");
			}

            Process p = pb.start();
			int exitCode = p.waitFor();
			if (exitCode != 0) {
				throw new NotificationException("schedblock annotate command failed with exit code: " + exitCode);
			}
        } catch (IOException e) {
            throw new NotificationException("Failed to issue JIRA notification: ", e);
        } catch (InterruptedException e) {
            throw new NotificationException(e);
		}
	}
	
}
