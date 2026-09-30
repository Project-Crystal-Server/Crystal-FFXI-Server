import mariadb


def migration_name():
    return "Adding client_version and client_expansions to accounts_sessions"


def check_preconditions(cur):
    return


def needs_to_run(cur):
    cur.execute("SHOW COLUMNS FROM accounts_sessions LIKE 'client_version'")
    if not cur.fetchone():
        return True
    return False


def migrate(cur, db):
    try:
        cur.execute("ALTER TABLE accounts_sessions \
                ADD COLUMN IF NOT EXISTS `client_version` varchar(16) NOT NULL DEFAULT '' AFTER `version_mismatch`, \
                ADD COLUMN IF NOT EXISTS `client_expansions` int(10) unsigned NOT NULL DEFAULT '0' AFTER `client_version`;")
        db.commit()
    except mariadb.Error as err:
        print("Something went wrong: {}".format(err))
