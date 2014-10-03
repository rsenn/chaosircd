CREATE TABLE `users` (
  `uid` varchar(64) NOT NULL,
  `msisdn` varchar(15) DEFAULT '',
  `imsi` varchar(15) DEFAULT '',
  `imei` varchar(15) DEFAULT '',
  `flag_level` int(11) DEFAULT '0',
  `flag_reason` varchar(256) DEFAULT '',
  `last_login` timestamp NULL DEFAULT CURRENT_TIMESTAMP,
  `last_location` point DEFAULT NULL,
  `password` varchar(256) DEFAULT '',
  PRIMARY KEY (`uid`),
  UNIQUE KEY `msisdn` (`msisdn`,`imsi`,`imei`),
  UNIQUE KEY `uid` (`uid`,`msisdn`,`imsi`,`imei`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
