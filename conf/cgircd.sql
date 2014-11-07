DROP TABLE IF EXISTS `users`;
CREATE TABLE `users` (
  `uid` varchar(64) NOT NULL,
  `msisdn` varchar(15) DEFAULT '',
  `imsi` varchar(15) DEFAULT '',
  `imei` varchar(15) DEFAULT '',
  `flag_level` int(11) DEFAULT '0',
  `flag_reason` varchar(256) DEFAULT '',
  `last_login` timestamp NULL DEFAULT CURRENT_TIMESTAMP,
  `last_location` point DEFAULT NULL,
  `phone_hw` varchar(255) DEFAULT '',
  `phone_os` varchar(255) DEFAULT '',
  PRIMARY KEY (`uid`),
  UNIQUE KEY `msisdn` (`msisdn`),
  UNIQUE KEY `imsi` (`imsi`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
