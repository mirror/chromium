L0: try { break L0; } catch (o) { }
L1: try { break L1; } finally { }

do { try { break; } catch (o) { } } while (false);
do { try { break; } finally { } } while (false);

do { try { continue; } catch (o) { } } while (false);
do { try { continue; } finally { } } while (false);
